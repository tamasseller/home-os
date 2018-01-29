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
		using namespace ArpPacket;

		auto self = static_cast<ReplyJob*>(item);
		auto interface = ArpCore::getInteface();

		NET_ASSERT(result == Result::Done);

		PacketBuilder builder(self->poolParams.allocator);

		Packet requestPacket;
		while(self->arpRequestQueue.take(requestPacket)) {
			PacketStream reader(requestPacket);

	        NET_ASSERT(reader.skipAhead(static_cast<uint16_t>(interface->getHeaderSize())));

			StructuredAccessor<SenderMac, SenderIp, TargetIp> requestAccessor;

			NET_ASSERT(requestAccessor.extract(reader));

			requestPacket.template dispose<Pool::Quota::Rx>();

			/*
			 * Check if the requested IP is associated with any routes out of this interface.
			 */
			Route* route = state.routingTable.findRouteWithSource(interface, requestAccessor.get<TargetIp>());
			if(!route) {
				continue;
			} else
				state.routingTable.releaseRoute(route);

			/*
			 * Fill ethernet header with the requester as the destination and write the ARP boilerplate.
			 */
			interface->ArpCore::resolver.fillHeader(builder, requestAccessor.get<SenderMac>(), 0x0806);

            StructuredAccessor<HType, PType, HLen, PLen, Operation, SenderMac, SenderIp, TargetMac, TargetIp> replyAccessor;
            replyAccessor.get<HType>() =      uint16_t(0x0001);
            replyAccessor.get<PType>() =      uint16_t(0x0800);
            replyAccessor.get<HLen>() =       0x06;
            replyAccessor.get<PLen>() =       0x04;
            replyAccessor.get<Operation>() =  Operation::Type::Reply;
            replyAccessor.get<SenderMac>() =  interface->ArpCore::resolver.getAddress();
            replyAccessor.get<SenderIp>() =   requestAccessor.get<TargetIp>();
            replyAccessor.get<TargetMac>() =  requestAccessor.get<SenderMac>();
            replyAccessor.get<TargetIp>() =   requestAccessor.get<SenderIp>();
            NET_ASSERT(replyAccessor.fill(builder));

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
