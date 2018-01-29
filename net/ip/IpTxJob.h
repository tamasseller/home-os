/*
 * IpTxJob.h
 *
 *  Created on: 2018.01.21.
 *      Author: tooma
 */

#ifndef IPTXJOB_H_
#define IPTXJOB_H_

#include "Network.h"

static constexpr const char arpReplyPreamble[8] = {0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x02};

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
template<class Child>
class Network<S, Args...>::IpTxJob: public Os::IoJob {
	friend class Os::IoChannel;

	/*
	 * Constant packet pieces and parameters for well-known management protocols.
	 */

protected:
	static constexpr size_t ipHeaderSize = 20;

private:
	static const size_t arpReqSize = 28;

	static const uint16_t etherTypeIp = static_cast<uint16_t>(0x0800);
	static const uint16_t etherTypeArp = static_cast<uint16_t>(0x0806);

	union {
	        /**
	         * The route the packet is to be sent over.
	         *
	         * It is used only until the headers are constructed, provides:
	         *
	         *  - The network interface that is used for:
	         *    - Querying the associated ARP cache.
	         *    - Sending the packet in stage 3.
	         *  - The source address to be specified in the IP header.
	         */
	        Route* route;
	        Interface* device;
	};

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

	union {
		/**
		 * Packet information used during the first stage: buffer pool request and
		 * reply holder, it holds the allocator used for the header and data assembly.
         */
		typename Pool::IoData stage1;
		/**
		 * Packet information used during the second stage: the assembled packet, filled
		 * with the headers at the end of stage 1 and during stage 2, the packet is also
		 * patched at the start of stage 3 with the size and possibly the checksum. */
        PacketBuilder stage2;

		/**
		 * Packet data used during the third, final stage: The final queueable packet,
		 * that is made from _stage2_. during stage 3.
         */
        PacketTransmissionRequest stage3;
	} packet;

	AddressIp4 getDestinationIpForL2() {
	    return route->isDirect() ? dst : route->getGateway();
	}

	static bool allocated(Launcher *launcher, IoJob* item, Result result)
	{
		auto self = static_cast<IpTxJob*>(item);
		auto route = self->route;
		Interface* dev = route->getDevice();
		state.routingTable.releaseRoute(route);

		if(result == Result::Done) {
			auto route = self->route;
			auto resolver = route->getDevice()->getResolver();
			auto allocator = self->packet.stage1.allocator;
			auto &packet = self->packet.stage2;

			packet.init(allocator);

			resolver->fillHeader(packet, self->arp.mac, etherTypeIp);

			Fixup::fillInitialIpHeader(packet, route->getSource(), self->dst);

			self->device = dev;

			return static_cast<Child*>(self)->onPreparationDone(launcher, item);
		} else
			return static_cast<Child*>(self)->onPreparationAborted(launcher, item, result);
	}

	/**
	 * Handler for the final completion of the L2 address resolution process.
	 */
	static bool addressResolved(Launcher *launcher, IoJob* item, Result result)
	{
		auto self = static_cast<IpTxJob*>(item);

		if(result == Result::Done) {
			auto route = self->route;
			auto resolver = route->getDevice()->getResolver();

			if(resolver && self->arp.mac == AddressEthernet::allZero) {
				if(self->retry--){
					/*
					 * Restart arp query packet generation and sending.
					 */
					const uint16_t size = bytesToBlocks(arpReqSize + route->getDevice()->getHeaderSize());


			    	return state.pool.template allocateDirectOrDeferred<Pool::Quota::Tx>(
			    			launcher, &IpTxJob::arpPacketAllocated, &self->packet.stage1, size);
				} else {
					/*
					 * Give up if number of retries exhausted.
					 */
					state.increment(&DiagnosticCounters::Ip::outputArpFailed);
					return static_cast<Child*>(self)->onArpTimeout(launcher, item);
				}
			}

			/*
			 * Allocate buffer for the business packet.
			 */
	    	return state.pool.template allocateDirectOrDeferred<Pool::Quota::Tx>(
	    			launcher, &IpTxJob::allocated, &self->packet.stage1, self->nBlocks);
		} else {
            state.routingTable.releaseRoute(self->route);
		    return static_cast<Child*>(self)->onPreparationAborted(launcher, item, result);
		}
	}

	/**
	 * Handler for completion of sending the arp query packet.
	 */
	static bool arpPacketSent(Launcher *launcher, IoJob* item, Result result)
	{
		auto self = static_cast<IpTxJob*>(item);

        /*
         * Dispose of the ARP request packet in the hope that the request is not needed to
         * be repeated (and even if it needs to be, avoid sitting on resources while waiting).
         */
        self->packet.stage3.template dispose<Pool::Quota::Tx>();

		if(result == Result::Done) {
			self->arp.ip = self->getDestinationIpForL2();

			state.increment(&DiagnosticCounters::Arp::outputSent);
			state.increment(&DiagnosticCounters::Arp::requestSent);

			launcher->launch(self->route->getDevice()->getResolver(), &IpTxJob::addressResolved, &self->arp);
			return true;
		} else {
            state.routingTable.releaseRoute(self->route);
		    return static_cast<Child*>(self)->onPreparationAborted(launcher, item, result);
		}
	}

	/**
	 * Handler for completion of buffer allocation for the ARP query packet.
	 */
	static bool arpPacketAllocated(Launcher *launcher, IoJob* item, Result result)
	{
	    using namespace ArpPacket;
		IpTxJob* self = static_cast<IpTxJob*>(item);

        auto route = self->route;

		if(result == Result::Done) {
			auto allocator = self->packet.stage1.allocator;
			auto &packet = self->packet.stage2;

			packet.init(allocator);

			route->getDevice()->getResolver()->fillHeader(packet, AddressEthernet::broadcast, etherTypeArp);

			StructuredAccessor<HType, PType, HLen, PLen, Operation, SenderMac, SenderIp, TargetMac, TargetIp> arpAccessor;
			arpAccessor.get<HType>() =      uint16_t(0x0001);
			arpAccessor.get<PType>() =      uint16_t(0x0800);
			arpAccessor.get<HLen>() =       0x06;
			arpAccessor.get<PLen>() =       0x04;
			arpAccessor.get<Operation>() =  Operation::Type::Request;
			arpAccessor.get<SenderMac>() =  route->getDevice()->getResolver()->getAddress();
			arpAccessor.get<SenderIp>() =   route->getSource();
			arpAccessor.get<TargetMac>() =  AddressEthernet::allZero;
			arpAccessor.get<TargetIp>() =   self->getDestinationIpForL2();
			NET_ASSERT(arpAccessor.fill(packet));

			packet.done();

			Packet copy = self->packet.stage2;
			self->packet.stage3.init(copy);

			state.increment(&DiagnosticCounters::Arp::outputQueued);

			launcher->launch(route->getDevice()->getSender(), &IpTxJob::arpPacketSent, &self->packet.stage3);
		    return true;
		} else {
            state.routingTable.releaseRoute(route);
		    return static_cast<Child*>(self)->onPreparationAborted(launcher, item, result);
		}
	}

	/*
	 * For sending
	 */
	static bool sent(Launcher *launcher, IoJob* item, Result result)
	{
		auto self = static_cast<IpTxJob*>(item);

		state.increment(&DiagnosticCounters::Ip::outputSent);

		return static_cast<Child*>(self)->onSent(launcher, item, result);
	}

protected:
	inline void postProcess(PacketStream&, DummyDigester&, size_t) {}

public:
	static const auto blockMaxPayload = Block::dataSize;

	/**
	 * Initiate the packet preparation sequence.
	 */
	static bool startPreparation(Launcher *launcher, IoJob* item, AddressIp4 dst, size_t nInLine, size_t nIndirect, uint8_t arpRetry)
	{
		auto self = static_cast<IpTxJob*>(item);

		state.increment(&DiagnosticCounters::Ip::outputRequest);

		/*
		 * Find the right network and save arguments.
		 */
		if(auto route = state.routingTable.findRouteTo(dst)) {
			self->route = route;
			self->dst = dst;

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
			 *                ___________________________/ \_________________________
			 *               /                                                       \
			 *               | arbitrary length user data accessed through reference |
			 */
			self->nBlocks = static_cast<uint8_t>(
					(nInLine - nIndirect + blockMaxPayload - 1) / blockMaxPayload +
					2 * nIndirect);

			/*
			 * Try to find the L2 address from the device cache, if found short
			 * circuit the ARP querying and cut-through to the completion of it.
			 */
			auto resolver = route->getDevice()->getResolver();
			if(!resolver || resolver->resolveAddress(self->getDestinationIpForL2(), self->arp.mac)) {
				return addressResolved(launcher, self, Result::Done);
			}

			/*
			 * If not found: allocate buffer for ARP query and set up retry counter.
			 */
			self->retry = arpRetry;
            const uint16_t size = bytesToBlocks(arpReqSize + route->getDevice()->getHeaderSize());

            return state.pool.template allocateDirectOrDeferred<Pool::Quota::Tx>(
                    launcher, &IpTxJob::arpPacketAllocated, &self->packet.stage1, size);
		} else {
			state.increment(&DiagnosticCounters::Ip::outputNoRoute);
		    return static_cast<Child*>(self)->onNoRoute(launcher, item);
		}
	}

	template<class PayloadDigester = DummyDigester>
	static bool startTransmission(Launcher *launcher, IoJob* item, uint8_t protocol, uint8_t ttl)
	{
		auto self = static_cast<IpTxJob*>(item);

		self->packet.stage2.done();
		Packet copy = self->packet.stage2;
		self->packet.stage3.init(copy);

		InetChecksumDigester headerChecksum;
		PayloadDigester payloadChecksum;

		uint16_t length = Fixup::headerFixupStepOne(
				self->packet.stage3,
				self->device->getHeaderSize(),
				headerChecksum,
				payloadChecksum,
				ttl,
				protocol);

		NET_ASSERT(length != (uint16_t)-1);

		PacketStream modifier(self->packet.stage3);

		Fixup::headerFixupStepTwo(
				modifier,
				self->device->getHeaderSize(),
				length,
				headerChecksum.result());

		for(size_t left = 8; left;) {
			Chunk chunk = modifier.getChunk();
			size_t run = left < chunk.length ? left : chunk.length;
			payloadChecksum.consume(chunk.start, run, left & 1);
			modifier.advance(static_cast<uint16_t>(run));
			left -= run;
		}

		NET_ASSERT(modifier.skipAhead(ipHeaderSize - 20));

		static_cast<Child*>(self)->postProcess(modifier, payloadChecksum, length - ipHeaderSize);

		state.increment(&DiagnosticCounters::Ip::outputQueued);

		launcher->launch(self->device->getSender(), &IpTxJob::sent, &self->packet.stage3);
		return true;
	}

	inline PacketBuilder& accessPacket() {
		return packet.stage2;
	}

	inline void disposePacket() {
		packet.stage3.template dispose<Pool::Quota::Tx>();
	}
};

#endif /* IPTXJOB_H_ */
