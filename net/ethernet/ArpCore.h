/*
 * ArpCore.h
 *
 *  Created on: 2018.01.22.
 *      Author: tooma
 */

#ifndef ARPCORE_H_
#define ARPCORE_H_

#include "Network.h"

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
		PacketStream reader(start);

		/*
		 * Skip destination ethernet header and initial fields of the ARP payload all the
		 * way to the sender hardware address, the initial fields are already processed at
		 * this point and are known to describe an adequate reply message.
		 */
		NET_ASSERT(reader.skipAhead(static_cast<uint16_t>(getInteface()->getHeaderSize() + 8)));

		AddressEthernet replyMac;
		NET_ASSERT(reader.copyOut(reinterpret_cast<char*>(replyMac.bytes), 6) == 6);

		AddressIp4 replyIp;
		NET_ASSERT(reader.read32net(replyIp.addr));

		/*
		 * Notify the ARP table about the resolved address.
		 */
		resolver.handleResolved(replyIp, replyMac, arpTimeout);

		/*
		 * Move reader to the end, then dispose of the reply packet.
		 */
		start.template dispose<Pool::Quota::Rx>();
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
		uint16_t opCode, hType, pType;
	    uint8_t hLen, pLen;

	    state.increment(&DiagnosticCounters::Arp::inputReceived);

	    PacketStream reader(packet);
		reader.skipAhead(getInteface()->getHeaderSize());

		if(!reader.read16net(hType) ||
		   !reader.read16net(pType) ||
	       !reader.read8(hLen) ||
	       !reader.read8(pLen))
			goto formatError;

		if(hType != 1 || hLen != 6 || pType != 0x800 || pLen != 4)
			goto formatError;

		if(!reader.read16net(opCode) || !reader.skipAhead(20))
			goto formatError;

		if(opCode == 0x01) {
			state.increment(&DiagnosticCounters::Arp::requestReceived);
			this->replyJob.handleArpRequest(packet);
		} else if(opCode == 0x0002) {
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
