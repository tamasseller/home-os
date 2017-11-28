/*
 * IpTransmission.h
 *
 *  Created on: 2017.11.26.
 *      Author: tooma
 */

#ifndef IPTRANSMISSION_H_
#define IPTRANSMISSION_H_

// <sender mac>, <sender ip>, <target mac>, <target ip>,

template<class S, class... Args>
struct Network<S, Args...>::IpTxJob: Os::IoJob {
	static constexpr const char arpRequestPreamble[8] = {0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x01};
	static constexpr const char arpReplyPreamble[8] = {0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x02};
	static constexpr const char zeroMac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	static const size_t arpReqSize = 28;
	static const size_t ipHeaderSize = 20;

	friend class Os::IoChannel;
	AddressIp4 dst;
	size_t length;
	const char* error;

	union {
		Route* route;
		Interface* device;
	};

	union {
		BufferPoolIoData<Os> poolParams;
		TxPacket packet;
	};

	ArpTableIoData arp;
	uint8_t retry;

	enum class AsyncResult {
		Done, Later, Error
	};

	/* TODO + header size */
	inline AsyncResult allocateBuffers(typename Os::IoJob::Hook hook, size_t size, typename Os::IoJob::Callback callback) {
		if(void *ret = route->dev->allocateBuffers(size)) {
			poolParams.first = ret;
			return callback(this, Os::IoJob::Result::Done, hook) ? AsyncResult::Later: AsyncResult::Done;
		}

		poolParams.size = size;
		return route->dev->requestAllocation(hook, this, callback, &poolParams) ? AsyncResult::Later : AsyncResult::Error;
	}

	static bool allocated(typename Os::IoJob* item, typename Os::IoJob::Result result, typename Os::IoJob::Hook hook) {
		auto self = static_cast<IpTxJob*>(item);

		if(result == Os::IoJob::Result::Done) {
			void* firstBlock = self->poolParams.first;
			self->packet.init(firstBlock, 0, self->route->dev->getStandardPacketOperations());
		}

		// TODO fill headers.

		Interface* dev = self->route->dev;
		state.routingTable.releaseRoute(self->route);
		self->device = dev;
		self->device->releaseAddress(self->arp.entry);

		return false;
	}

	/**
	 * Handler for the final completion of the L2 address resolution process.
	 */
	static bool addressResolved(typename Os::IoJob* item, typename Os::IoJob::Result result, typename Os::IoJob::Hook hook)
	{
		auto self = static_cast<IpTxJob*>(item);

		if(result == Os::IoJob::Result::Done) {
			/*
			 * Allocate buffer for the business packet.
			 */
			AsyncResult ret = self->allocateBuffers(hook, self->length /* TODO + header size */, &IpTxJob::allocated);

			if(ret != AsyncResult::Error)
				return ret == AsyncResult::Later;

		} else if(self->retry--){
			/*
			 * Restart arp query packet generation and sending.
			 */
			AsyncResult ret = self->allocateBuffers(hook, arpReqSize, &IpTxJob::arpPacketAllocated);

			if(ret != AsyncResult::Error)
				return ret == AsyncResult::Later;

		} else {
			/*
			 * Give up if number of retries exhausted.
			 */
			self->error = NetErrorStrings::unresolved;
			return false;
		}

		/* LCOV_EXCL_START: Beginning of a block that is not intended to be executed. */
		self->error = NetErrorStrings::unknown;
		return false;
		/* LCOV_EXCL_STOP: End of the block that is not intended to be executed. */
	}

	/**
	 * Handler for completion of sending the arp query packet.
	 */
	static bool arpPacketSent(typename Os::IoJob* item, typename Os::IoJob::Result result, typename Os::IoJob::Hook hook)
	{
		auto self = static_cast<IpTxJob*>(item);

		if(result == Os::IoJob::Result::Done) {
			/*
			 * Dispose of the arp request packet in the hope that the request is not needed to
			 * be repeated (and even if it needs to be, avoid sitting on resources while waiting).
			 */
			self->packet.dispose();

			/*
			 * Wait for the required address.
			 */
			self->arp.ip = self->dst;
			if(self->route->dev->requestResolution(hook, self, &IpTxJob::addressResolved, &self->arp))
				return true;
		}

		/* LCOV_EXCL_START: Beginning of a block that is not intended to be executed. */
		self->error = NetErrorStrings::unknown;
		return false;
		/* LCOV_EXCL_STOP: End of the block that is not intended to be executed. */
	}

	/**
	 * Handler for completion of buffer allocation for the arp query packet.
	 */
	static bool arpPacketAllocated(typename Os::IoJob* item, typename Os::IoJob::Result result, typename Os::IoJob::Hook hook)
	{
		IpTxJob* self = static_cast<IpTxJob*>(item);

		if(result == Os::IoJob::Result::Done) {
			void* firstBlock = self->poolParams.first;
			self->packet.init(firstBlock, 0, self->route->dev->getStandardPacketOperations());

			self->packet.copyIn(arpReplyPreamble, sizeof(arpReplyPreamble));
			const AddressEthernet &senderMac = self->route->dev->getAddress();
			uint32_t senderIpNetOrder = correctEndian(self->route->getSource().addr);
			uint32_t targetIpNetOrder = correctEndian(self->dst.addr);
			self->packet.copyIn(reinterpret_cast<const char*>(&senderMac.bytes[0]), 6);
			self->packet.copyIn32(senderIpNetOrder);
			self->packet.copyIn(zeroMac, 6);
			self->packet.copyIn32(targetIpNetOrder);

			// TODO fill arp query

			if(self->submit(hook, self->route->dev->getSender(), &IpTxJob::arpPacketSent, &self->packet))
				return true;
		}

		/* LCOV_EXCL_START: Beginning of a block that is not intended to be executed. */
		self->error = NetErrorStrings::unknown;
		return false;
		/* LCOV_EXCL_STOP: End of the block that is not intended to be executed. */
	}

	/**
	 * Initiate the packet preparation sequence.
	 */
	inline bool start(AddressIp4 dst, size_t length, typename Os::IoJob::Hook hook = 0)
	{
		error = nullptr;

		/*
		 * Find the right network and save arguments.
		 */
		if(auto route = state.routingTable.findRoute(dst)) {
			this->route = route;
			this->dst = dst;
			this->length = length;

			/*
			 * Try to find the L2 address from the device cache, if found short
			 * circuit the ARP querying and cut-through to the completion of it.
			 */
			if(ArpEntry* entry = this->route->dev->resolveAddress(dst)) {
				this->arp.entry = entry;
				addressResolved(this, Os::IoJob::Result::Done, hook);
				return true;
			}

			/*
			 * If not found: allocate buffer for ARP query and set up retry counter.
			 */
			this->retry = arpRequestRetry;
			if(allocateBuffers(hook, arpReqSize, &IpTxJob::arpPacketAllocated) != AsyncResult::Error)
				return true;

			/* LCOV_EXCL_START: Beginning of a block that is not intended to be executed. */
			error = NetErrorStrings::alreadyUsed;
			/* LCOV_EXCL_STOP: End of the block that is not intended to be executed. */
		} else
			error = NetErrorStrings::noRoute;

		return false;
	}

	/*
	 * For sending
	 */
	static bool sent(typename Os::IoJob* item, typename Os::IoJob::Result result, typename Os::IoJob::Hook hook) {
		auto self = static_cast<IpTxJob*>(item);
		self->packet.dispose();
		return false;
	}

	inline bool start(typename Os::IoJob::Hook hook = 0) {
		return this->submit(hook, this->device->getSender(), &IpTxJob::sent, &packet);
	}
};

template<class S, class... Args>
class Network<S, Args...>::IpTransmission: public Os::template IoRequest<IpTxJob> {
public:
	inline bool prepare(AddressIp4 dst, size_t size) {
		if(this->shouldWait())
			this->wait();

		return this->start(dst, size);
	}

	inline bool send() {
		if(this->shouldWait())
			this->wait();

		if(this->error)
			return false;

		return this->start();
	}

	inline bool fill(const char* data, size_t length) {
		if(this->shouldWait())
			this->wait();

		if(this->error)
			return false;

		return this->packet.copyIn(data, length);
	}

	inline const char* getError() {
		return this->error;
	}
};

template<class S, class... Args>
constexpr const char Network<S, Args...>::IpTxJob::arpRequestPreamble[];

template<class S, class... Args>
constexpr const char Network<S, Args...>::IpTxJob::arpReplyPreamble[];

template<class S, class... Args>
constexpr const char Network<S, Args...>::IpTxJob::zeroMac[];


#endif /* IPTRANSMISSION_H_ */
