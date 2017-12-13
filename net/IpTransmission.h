/*
 * IpTransmitter.h
 *
 *  Created on: 2017.11.26.
 *      Author: tooma
 */

#ifndef IPTRANSMISSION_H_
#define IPTRANSMISSION_H_

#include "InetChecksumDigester.h"
#include "IpFixup.h"

/**
 * Multi-operation composite I/O job object for sending a simple "raw"
 * IP packet to a unicast destination.
 *
 * This object implements two asynchronous operations that must be invoked
 * in turns, and the user data filled in between, thus there are three
 * stages that the object goes through for every packet sent:
 *
 *  1. Preparation of a new packet: address lookup and buffer allocation.
 *  2. User data input: using in-line data copied from used objects or
 *     indirect data referring the data objects of the user (with optional
 *     life-cycle management support).
 *  3. Transmission of the packet.
 *
 * The first operation requires the user to specify the destination address and
 * the maximal size for the packet. The packet size has to be specified in order
 * to avoid deadlocks on simultaneous buffer allocations: the buffers needed to
 * store the specified maximal amount of data are allocated in advance, if the
 * allocation request can not be completed immediately, then the I/O job is scheduled
 * for asynchronous execution. The whole operation comprises of the following steps:
 *
 *   1. Route lookup and link layer address lookup:
 *
 *     - A suitable route is looked up, if not found and the default route is
 *       configured it is used throughout following the steps.
 *     - The resolution of the cached link layer address attempted using the ARP
 *       cache for the interface.
 *     - Buffers are allocated for the next operation, which depending on whether a
 *       matching entry is found in the ARP table or not, can either be:
 *
 *         b. For known destination: filling in link and initial IP headers (step 5).
 *         a. Otherwise: construction and sending of the ARP request packet (step 2).
 *
 *       These operations may or may not be completed immediately depending on the
 *       current buffer availability, in either case the handler for the completion
 *       is invoked with identical parameters (ie. it does not know if it is called
 *       directly or as an asynchronous callback).
 *
 *   2. Transmission of the ARP request: when the buffer allocation is complete, the
 *      contents of the request packet are generated. Then the packet is sent out over
 *      the interface selected in step 1.
 *
 *   3. Releasing of the request packet buffers: after the request is sent the buffers
 *      used to hold the request packet are freed up, in the hope that if the
 *      destination really exists on the network, then it will probably catch the
 *      message for at first go, so it won't be needed again.
 *
 *   4. Waiting for ARP reply: the next step is bound to the resolution of the
 *      requested address, but a timeout for this operation is also specified:
 *
 *      - If the lookup is successful allocate buffers for the user data as in step1,
 *        and after that completes progress to step 5.
 *      - If timed out, steps 2-4 are repeated, by first allocating new buffers
 *        for the request packet. This repeated re-transmission is done only
 *        a limited number of times specified by the _ArpRequestRetry_ parameter.
 *      - Fail if the re-transmission is done too many times.
 *
 *   5. Filling the headers: link layer and IP headers are filled with their
 *      initial values, the checksum field is left zero.
 *
 *  After completing the initial, preparation operation the user can add its own
 *  business data into the packet that is stored in the buffers allocated during
 *  the preparation operation. All of the methods provided for this activity are
 *  synchronous and non-blocking.
 *
 *  When the user is finished with filling in the data, the second, finalizing
 *  asynchronous operation is invoked on the object which:
 *
 *    1. Finalizes and queues the packet:
 *
 *         - Frees up leftover buffers, that where not used for storing user content.
 *         - Calculates writes the final packet length and calculates the checksum of
 *           the IP header, if the interface does not provide hardware off-loading
 *           for that.
 *         - Queues the packet for transmission.
 *
 *    2. Buffer release: after the packet has been transmitted the packet contents are
 *       no longer required to be kept, thus are freed up and reclaimed by the pool.
 */
template<class S, class... Args>
struct Network<S, Args...>::IpTxJob: Os::IoJob {
	friend class Os::IoChannel;
	using IoJob = typename Os::IoJob;
	using Result = typename IoJob::Result;
	using Launcher = typename IoJob::Launcher;
	using Callback = typename IoJob::Callback;

	/*
	 * Constant packet pieces and parameters for well-known management protocols.
	 */

	static constexpr const char arpRequestPreamble[8] = {0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x01};
	static constexpr const char arpReplyPreamble[8] = {0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x02};

	static constexpr size_t ipHeaderSize = 20;
	static const size_t arpReqSize = 28;

	static const uint16_t etherTypeIp = static_cast<uint16_t>(0x0800);
	static const uint16_t etherTypeArp = static_cast<uint16_t>(0x0806);

	/*
	 * Data fields that store information during the various steps of the process.
	 */
	const char* error;

	union {
		struct {
			/**
			 * The route the packed is to be sent over.
			 *
			 * It is used only until the headers are constructed, provides:
			 *
			 *  - The network interface that is used for:
			 *    - Querying the associated ARP cache.
			 *    - Sending the packet in stage 3.
			 *  - The source address to be specified in the IP header.
			 */
			Route* route;

			/**
			 * Destination IP address.
			 */
			AddressIp4 dst;

			/**
			 * ARP resolution request and reply holder, provides the
			 * destination MAC address after the resolution is done.
			 */
			ArpTableIoData<Os> arp;

			/**
			 * ARP retry counter.
			 */
			uint8_t retry;

			/**
			 * Number of blocks to be allocated for user data.
			 */
			uint8_t nBlocks;
		} stage1;


		struct {
			/**
			 * The device over which the packet is sent.
			 *
			 * It is used after the headers are done but before the
			 * packet is sent (stage 2, 3).
			 */
			Interface* device;
		} stage23;
	} transmission;

	union {
		/**
		 * Packet information used during the first stage.
		 */
		struct {
			/**
			 * Buffer pool request and reply holder, it holds the allocator
			 * used for the header and data assembly.
			 */
			typename Pool::IoData poolParams;
		} stage1;

		/**
		 * Packet information used during the second stage.
		 */
		struct {
			/**
			 * The assembled packet, filled with the headers at the end of
			 * stage 1 and during stage 2, the packet is also patched at the
			 * start of stage 3 with the size and possibly the checksum.
			 */
			TxPacketBuilder packet;
		} stage2;

		/**
		 * Packet data used during the third, final stage.
		 */
		struct {
			/**
			 * The final queueable packet, that is made from _stage12.packet_.
			 * during stage 3.
			 */
			PacketTransmissionRequest packet;
		} stage3;
	} packet;

	enum class AsyncResult {
		Done, Later, Error
	};

	inline AsyncResult allocateBuffers(Launcher *launcher, size_t size, Callback callback)
	{
		// This branch is not supposed to be taken ( LCOV_EXCL_START ).
		if(size > UINT16_MAX) {
			error = NetErrorStrings::allocError;
			return AsyncResult::Error;
		}
		// LCOV_EXCL_STOP

		auto ret = state.pool.template allocateDirect<Pool::Quota::Tx>(size);

		if(ret.hasMore()) {
			packet.stage1.poolParams.allocator = ret;
			return callback(launcher, this, Result::Done) ? AsyncResult::Later: AsyncResult::Done;
		}

		packet.stage1.poolParams.request.size = static_cast<uint16_t>(size);
		packet.stage1.poolParams.request.quota = Pool::Quota::Tx;

		return launcher->launch(&state.pool, callback, &packet.stage1.poolParams)
				? AsyncResult::Later : AsyncResult::Error;
	}

	static bool allocated(Launcher *launcher, IoJob* item, Result result) {
		auto self = static_cast<IpTxJob*>(item);
		auto route = self->transmission.stage1.route;
		Interface* dev = route->getDevice();
		state.routingTable.releaseRoute(route);

		if(result != Result::Done) {
			// assert(result == Result::Canceled);
			self->error = NetErrorStrings::genericCancel;
		} else {
			auto route = self->transmission.stage1.route;
			auto resolver = route->getDevice()->getResolver();
			auto allocator = self->packet.stage1.poolParams.allocator;
			auto &packet = self->packet.stage2.packet;

			packet.init(allocator);

			resolver->fillHeader(packet, self->transmission.stage1.arp.mac, etherTypeIp);

			fillInitialIpHeader(packet, route->getSource(), self->transmission.stage1.dst);

			self->transmission.stage23.device = dev;
		}

		return false;
	}

	/**
	 * Handler for the final completion of the L2 address resolution process.
	 */
	static bool addressResolved(Launcher *launcher, IoJob* item, Result result)
	{
		auto self = static_cast<IpTxJob*>(item);

		if(result == Result::Done) {
			auto route = self->transmission.stage1.route;
			auto resolver = route->getDevice()->getResolver();

			if(resolver && self->transmission.stage1.arp.mac == AddressEthernet::allZero) {
				if(self->transmission.stage1.retry--){
					/*
					 * Restart arp query packet generation and sending.
					 */
					const auto size = (arpReqSize + route->getDevice()->getHeaderSize() + blockMaxPayload - 1) / blockMaxPayload;
					AsyncResult ret = self->allocateBuffers(launcher, size, &IpTxJob::arpPacketAllocated);

					if(ret != AsyncResult::Error)
						return ret == AsyncResult::Later;

				} else {
					/*
					 * Give up if number of retries exhausted.
					 */
					self->error = NetErrorStrings::unresolved;
					return false;
				}
			}

			/*
			 * Allocate buffer for the business packet.
			 */
			AsyncResult ret = self->allocateBuffers(launcher, self->transmission.stage1.nBlocks, &IpTxJob::allocated);

			if(ret != AsyncResult::Error)
				return ret == AsyncResult::Later;

		}
		/* LCOV_EXCL_START: Beginning of a block that is not intended to be executed. */
		self->error = NetErrorStrings::unknown;
		return false;
		/* LCOV_EXCL_STOP: End of the block that is not intended to be executed. */
	}

	/**
	 * Handler for completion of sending the arp query packet.
	 */
	static bool arpPacketSent(Launcher *launcher, IoJob* item, Result result)
	{
		auto self = static_cast<IpTxJob*>(item);

		if(result == Result::Done) {
			/*
			 * Dispose of the ARP request packet in the hope that the request is not needed to
			 * be repeated (and even if it needs to be, avoid sitting on resources while waiting).
			 */
			self->packet.stage3.packet.template dispose<Pool::Quota::Tx>();

			/*
			 * Wait for the required address.
			 */
			self->transmission.stage1.arp.ip = self->transmission.stage1.dst;
			if(launcher->launch(self->transmission.stage1.route->getDevice()->getResolver(), &IpTxJob::addressResolved, &self->transmission.stage1.arp))
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
	static bool arpPacketAllocated(Launcher *launcher, IoJob* item, Result result)
	{
		IpTxJob* self = static_cast<IpTxJob*>(item);

		if(result == Result::Done) {
			auto allocator = self->packet.stage1.poolParams.allocator;
			auto route = self->transmission.stage1.route;
			const uint32_t senderIp = route->getSource().addr;
			const uint32_t targetIp = self->transmission.stage1.dst.addr;
			const AddressEthernet &senderMac = route->getDevice()->getResolver()->getAddress();
			auto &packet = self->packet.stage2.packet;

			packet.init(allocator);
			route->getDevice()->getResolver()->fillHeader(packet, AddressEthernet::broadcast, etherTypeArp);
			packet.copyIn(arpRequestPreamble, sizeof(arpRequestPreamble));
			packet.copyIn(reinterpret_cast<const char*>(&senderMac.bytes[0]), 6);
			packet.write32net(senderIp);
			packet.copyIn(reinterpret_cast<const char*>(&AddressEthernet::allZero.bytes[0]), 6);
			packet.write32net(targetIp);
			packet.done();

			Packet copy = self->packet.stage2.packet;
			self->packet.stage3.packet.init(copy);

			if(launcher->launch(route->getDevice()->getSender(), &IpTxJob::arpPacketSent, &self->packet.stage3.packet))
				return true;
		}

		/* LCOV_EXCL_START: Beginning of a block that is not intended to be executed. */
		self->error = NetErrorStrings::unknown;
		return false;
		/* LCOV_EXCL_STOP: End of the block that is not intended to be executed. */
	}

	/*
	 * For sending
	 */
	static bool sent(Launcher *launcher, IoJob* item, Result result)
	{
		auto self = static_cast<IpTxJob*>(item);

		self->packet.stage3.packet.template dispose<Pool::Quota::Tx>();
		return false;
	}

public:
	static const auto blockMaxPayload = Block::dataSize;

	/**
	 * Initiate the packet preparation sequence.
	 */
	static bool startPreparation(Launcher *launcher, IoJob* item, AddressIp4 dst, size_t nInLine, size_t nIndirect)
	{
		auto self = static_cast<IpTxJob*>(item);

		self->error = nullptr;

		/*
		 * Find the right network and save arguments.
		 */
		if(auto route = state.routingTable.findRouteTo(dst)) {
			self->transmission.stage1.route = route;
			self->transmission.stage1.dst = dst;

			nInLine += ipHeaderSize + route->getDevice()->getHeaderSize();
			/*
			 * The final number of buffers is determined not only by the amount of in-line data
			 * but also the indirect data references. If in-line and indirect buffers are used
			 * on after the other, the indirect one cause the previous in-line to not be filled
			 * completely, so the tightest upper limit on the number of buffers consists of the
			 * two buffers for each indirect plus the minimal number of buffers that can host
			 * the in-line data minus one byte for each indirect one, because in the worst case
			 * an in-line buffer preceding an indirect one contains a single byte.
			 *
			 * |some in-| -> |line dat| -> |a_______| -> | | -> |some oth| -> |er data_|
			 *                                 ^          |
			 *                                 |          |
			 * Truncated block (one byte wc) --/          |
			 *                ___________________________/ \____________________________
			 *               /                                                          \
			 *               | arbitrary length user data accessed through by reference |
			 */
			self->transmission.stage1.nBlocks = static_cast<uint8_t>(
					(nInLine - nIndirect + blockMaxPayload - 1) / blockMaxPayload +
					2 * nIndirect);

			/*
			 * Try to find the L2 address from the device cache, if found short
			 * circuit the ARP querying and cut-through to the completion of it.
			 */
			auto resolver = route->getDevice()->getResolver();
			if(!resolver || resolver->resolveAddress(dst, self->transmission.stage1.arp.mac)) {
				addressResolved(launcher, self, Result::Done);
				return true;
			}

			/*
			 * If not found: allocate buffer for ARP query and set up retry counter.
			 */
			self->transmission.stage1.retry = arpRequestRetry;
			const auto size = (arpReqSize + route->getDevice()->getHeaderSize() + blockMaxPayload - 1) / blockMaxPayload;
			if(self->allocateBuffers(launcher, size, &IpTxJob::arpPacketAllocated) != AsyncResult::Error)
				return true;

			/* LCOV_EXCL_START: Beginning of a block that is not intended to be executed. */
			self->error = NetErrorStrings::alreadyUsed;
			/* LCOV_EXCL_STOP: End of the block that is not intended to be executed. */
		} else
			self->error = NetErrorStrings::noRoute;

		return false;
	}

	static bool startTransmission(Launcher *launcher, IoJob* item, uint8_t protocol, uint8_t ttl)
	{
		auto self = static_cast<IpTxJob*>(item);

		self->packet.stage2.packet.done();
		Packet copy = self->packet.stage2.packet;
		self->packet.stage3.packet.init(copy);

		InetChecksumDigester headerChecksum;
		DummyDigester payloadChecksum;

		uint16_t length = headerFixupStepOne(
				self->packet.stage3.packet,
				ttl,
				protocol,
				self->transmission.stage23.device->getHeaderSize(),
				ipHeaderSize,
				headerChecksum,
				payloadChecksum);

		if(length == (size_t)-1) {
			self->error = NetErrorStrings::unknown;
			return false;
		}

		PacketStream modifier;
		modifier.init(self->packet.stage3.packet);

		headerFixupStepTwo(
				modifier,
				self->transmission.stage23.device->getHeaderSize(),
				length,
				headerChecksum.result());

		return launcher->launch(self->transmission.stage23.device->getSender(), &IpTxJob::sent, &self->packet.stage3.packet);
	}
};

template<class S, class... Args>
class Network<S, Args...>::IpTransmitter: public Os::template IoRequest<IpTxJob> {
	bool check() {
		if(this->isOccupied())
			this->wait();

		return this->error == nullptr;
	}
public:
	inline bool prepare(AddressIp4 dst, size_t inLineSize, size_t indirectCount = 0, size_t timeout = 100)
	{
		check();
		return this->launch(&IpTxJob::startPreparation, dst, inLineSize, indirectCount);
	}

	inline bool send(uint8_t protocol, uint8_t ttl = 64)
	{
		if(!check())
			return false;

		return this->launch(&IpTxJob::startTransmission, protocol, ttl);
	}

	inline uint16_t fill(const char* data, uint16_t length)
	{
		if(!check())
			return false;

		return this->packet.stage2.packet.copyIn(data, length);
	}

    inline bool addIndirect(const char* data, uint16_t length, typename Block::Destructor destructor = nullptr, void* userData = nullptr) {
		if(!check())
			return false;

        return this->packet.stage2.packet.addByReference(data, length, destructor, userData);
    }

	inline const char* getError() {
		return this->error;
	}
};

template<class S, class... Args>
constexpr const char Network<S, Args...>::IpTxJob::arpRequestPreamble[];

template<class S, class... Args>
constexpr const char Network<S, Args...>::IpTxJob::arpReplyPreamble[];

#endif /* IPTRANSMISSION_H_ */
