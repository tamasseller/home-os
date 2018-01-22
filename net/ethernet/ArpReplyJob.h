/*
 * ArpReplyJob.h
 *
 *  Created on: 2018.01.22.
 *      Author: tooma
 */

#ifndef ARPREPLYJOB_H_
#define ARPREPLYJOB_H_

#include "Network.h"

template<class S, class... Args>
template<class Driver>
struct Network<S, Args...>::ArpCore<Driver>::ReplyJob: public Os::IoJob {
    PacketChain arpRequestQueue;

    union {
    	typename Pool::IoData poolParams;
    	PacketTransmissionRequest txReq;
    };

    static inline bool replySent(Launcher *launcher, IoJob* item, Result result)
	{
		auto self = static_cast<ReplyJob*>(item);
		auto interface = ArpCore::getInteface();

    	NET_ASSERT(result == Result::Done);

		state.increment(&DiagnosticCounters::Arp::replySent);
		state.increment(&DiagnosticCounters::Arp::outputSent);

		self->txReq.template dispose<Pool::Quota::Tx>();

		if(!self->arpRequestQueue.isEmpty()) {
			startReplySequence(launcher, item);
			return true;
		}

		return false;
	}

	static inline bool assembleReply(Launcher *launcher, IoJob* item, Result result)
	{
		auto self = static_cast<ReplyJob*>(item);
		auto interface = ArpCore::getInteface();

		NET_ASSERT(result == Result::Done);

		PacketBuilder builder(self->poolParams.allocator);

		Packet requestPacket;
		while(self->arpRequestQueue.take(requestPacket)) {
			PacketStream reader(requestPacket);

			/*
			 * Skip destination ethernet header and initial fields of the ARP payload all the
			 * way to the sender hardware address, the initial fields are already processed at
			 * this point and are known to describe an adequate request. The packet is already
			 * checked for having enough payload content to do this, so this must not fail.
			 */
	        NET_ASSERT(reader.skipAhead(static_cast<uint16_t>(interface->getHeaderSize() + 8)));

			/*
			 * Save the source address of the request for later use as the destination of the reply.
			 */
			AddressEthernet requesterMac;
			NET_ASSERT(reader.copyOut(reinterpret_cast<char*>(requesterMac.bytes), 6) == 6);

			/*
			 * Read sender IP address.
			 */
			char requesterIp[4];
			NET_ASSERT(reader.copyOut(requesterIp, 4) == 4);
			NET_ASSERT(reader.skipAhead(static_cast<uint16_t>(6)));

			/*
			 * Read sender IP address.
			 */
			AddressIp4 queriedIp;
			NET_ASSERT(reader.read32net(queriedIp.addr));

			/*
			 * Check if the requested
			 */
			Route* route = state.routingTable.findRouteWithSource(interface, queriedIp);
			if(!route) {
				requestPacket.template dispose<Pool::Quota::Rx>();
				continue;
			} else
				state.routingTable.releaseRoute(route);

			/*
			 * Fill ethernet header with the requester as the destination and write the ARP boilerplate.
			 */
			interface->ArpCore::resolver.fillHeader(builder, requesterMac, 0x0806);

			static constexpr const char replyBoilerplate[] = {
				0x00, 0x01, // hwType
				0x08, 0x00, // protoType
				0x06,		// hSize
				0x04, 		// pSize
				0x00, 0x02	// opCode
			};

			NET_ASSERT(builder.copyIn(replyBoilerplate, sizeof(replyBoilerplate)) == sizeof(replyBoilerplate));

			/*
			 * Write source address again.
			 */
			NET_ASSERT(builder.copyIn(reinterpret_cast<const char*>(interface->ArpCore::resolver.getAddress().bytes), 6) == 6);

			/*
			 * Write source IP address.
			 */
			NET_ASSERT(builder.write32net(queriedIp.addr));

			/*
			 * Write destination address again.
			 */
			NET_ASSERT(builder.copyIn(reinterpret_cast<char*>(requesterMac.bytes), 6) == 6);

			/*
			 * Write destination IP address.
			 */
			NET_ASSERT(builder.copyIn(requesterIp, 4) == 4);

			requestPacket.template dispose<Pool::Quota::Rx>();

			builder.done();
			self->txReq.init(builder);

			state.increment(&DiagnosticCounters::Arp::outputQueued);
			launcher->launch(interface->getSender(), &ReplyJob::replySent, &self->txReq);
	        return true;
		}

		builder.done();
		builder.template dispose<Pool::Quota::Tx>();
		return false;
	}

	static inline bool startReplySequence(Launcher *launcher, IoJob* item)
	{
		auto self = static_cast<ReplyJob*>(item);
		auto interface = ArpCore::getInteface();

		static const uint16_t arpReplySize = 28;
		static const uint16_t bufferCount = bytesToBlocks(interface->getHeaderSize() + arpReplySize);

		return state.pool.template allocateDirectOrDeferred<Pool::Quota::Tx>(
				launcher,
				&ReplyJob::assembleReply,
				&self->poolParams,
				bufferCount);
	}

public:
	inline void handleArpRequest(Packet packet) {
		this->arpRequestQueue.put(packet);
		static_cast<ReplyJob*>(this)->launch(&ReplyJob::startReplySequence);
	}
};

#endif /* ARPREPLYJOB_H_ */
