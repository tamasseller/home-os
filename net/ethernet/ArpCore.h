/*
 * ArpCore.h
 *
 *  Created on: 2018.01.22.
 *      Author: tooma
 */

#ifndef ARPCORE_H_
#define ARPCORE_H_

#include "Network.h"

#include "ArpPacket.h"

template<class S, class... Args>
template<class Driver>
struct Network<S, Args...>::ArpCore: private PacketProcessor {
	friend PacketProcessor;
	class ReplyJob;
	class Resolver;

	ReplyJob replyJob;
    Resolver resolver;

    static constexpr inline auto* getInteface() {
    	return static_cast<Ethernet<Driver>*>(&Network<S, Args...>::state.interfaces);
    }

	inline void processArpReplyPacket(Packet start)
	{
		using namespace ArpPacket;

		PacketStream reader(start);

		NET_ASSERT(reader.skipAhead(static_cast<uint16_t>(getInteface()->getHeaderSize())));

		StructuredAccessor<SenderMac, SenderIp> accessor;

		NET_ASSERT(accessor.extract(reader));

		start.template dispose<Pool::Quota::Rx>();

		resolver.handleResolved(accessor.get<SenderIp>(), accessor.get<SenderMac>(), arpTimeout);
	}

public:
	inline ArpCore(): PacketProcessor(PacketProcessor::template make<ArpCore, &ArpCore::processArpReplyPacket>()) {}

	inline void init() {
		resolver.init();
	}

    inline void ageContent() {
    	resolver.ageContent();
    }

	inline void arpPacketReceived(Packet packet)
	{
		using namespace ArpPacket;

	    state.increment(&DiagnosticCounters::Arp::inputReceived);

	    PacketStream reader(packet);
		reader.skipAhead(getInteface()->getHeaderSize());

		StructuredAccessor<HType, PType, HLen, PLen, Operation, End> accessor;

		if(!accessor.extract(reader))
			goto formatError;

		if(accessor.get<HType>() != 1 || accessor.get<HLen>() != 6 ||
				accessor.get<PType>() != 0x800 || accessor.get<PLen>() != 4)
			goto formatError;

		if(accessor.get<Operation>() == Operation::Type::Request) {
			state.increment(&DiagnosticCounters::Arp::requestReceived);
			this->replyJob.handleArpRequest(packet);
		} else if(accessor.get<Operation>() == Operation::Type::Reply) {
			state.increment(&DiagnosticCounters::Arp::replyReceived);
			this->process(packet);
		} else {
			goto formatError;
		}

		return;

	formatError:
		state.increment(&DiagnosticCounters::Arp::inputFormatError);
		packet.template dispose<Pool::Quota::Rx>();
	}
};

#endif /* ARPCORE_H_ */
