/*
 * IpTransmission.h
 *
 *  Created on: 2017.11.26.
 *      Author: tooma
 */

#ifndef IPTRANSMISSION_H_
#define IPTRANSMISSION_H_

template<class S, class... Args>
struct Network<S, Args...>::IpTxJob: Os::IoJob {
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
			self->poolParams.size = self->length /* TODO + header size */;
			if(self->submit(hook, self->route->dev->getAllocator(), &IpTxJob::allocated, &self->poolParams))
				return true;

		} else if(self->retry--){
			/*
			 * Restart arp query packet generation and sending.
			 */
			self->poolParams.size = arpReqSize /* TODO + header size */;
			if(self->submit(hook, self->route->dev->getAllocator(), &IpTxJob::arpPacketAllocated, &self->poolParams))
				return true;

		} else {
			/*
			 * Give up if number of retries exhausted.
			 */
			self->error = NetErrorStrings::unresolvedError;
			return false;
		}

		self->error = NetErrorStrings::unknownError;
		return false;
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

		self->error = NetErrorStrings::unknownError;
		return false;
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

			// TODO fill arp query

			if(self->submit(hook, self->route->dev->getSender(), &IpTxJob::arpPacketSent, &self->packet))
				return true;
		}

		self->error = NetErrorStrings::unknownError;
		return false;
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
				return addressResolved(this, Os::IoJob::Result::Done, hook);
			}

			/*
			 * If not found: allocate buffer for ARP query and set up retry counter.
			 */
			this->retry = arpRequestRetry;
			this->poolParams.size = arpReqSize /* TODO + header size */;
			if(this->submit(hook, this->route->dev->getAllocator(), &IpTxJob::arpPacketAllocated, &this->poolParams))
				return true;

			error = NetErrorStrings::alreadyUsedError;
		} else
			error = NetErrorStrings::noRouteError;

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
};

#endif /* IPTRANSMISSION_H_ */
