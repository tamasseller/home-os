/*
 * Arp.h
 *
 *  Created on: 2017.12.21.
 *      Author: tooma
 */

#ifndef ARP_H_
#define ARP_H_

#include "Ethernet.h"

template<class S, class... Args>
template<class Driver>
inline void Network<S, Args...>::Ethernet<Driver>::arpPacketReceived(Packet packet)
{
	uint16_t opCode, hType, pType;
    uint8_t hLen, pLen;

    state.increment(&DiagnosticCounters::Arp::inputReceived);

    PacketStream reader(packet);
	reader.skipAhead(static_cast<Interface*>(this)->getHeaderSize());

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
		this->arpRequestQueue.put(packet);
		static_cast<ArpReplyJob*>(this)->launch(&startReplySequence);
	} else if(opCode == 0x0002) {
		state.increment(&DiagnosticCounters::Arp::replyReceived);
		static_cast<PacketProcessor*>(this)->process(packet);
	} else {
		goto formatError;
	}

	return;

formatError:
	state.increment(&DiagnosticCounters::Arp::inputFormatError);
	packet.template dispose<Pool::Quota::Rx>();
}

template<class S, class... Args>
template<class Driver>
inline void Network<S, Args...>::Ethernet<Driver>::processArpReplyPacket(Packet start)
{
    PacketStream reader(start);

	/*
	 * Skip destination ethernet header and initial fields of the ARP payload all the
	 * way to the sender hardware address, the initial fields are already processed at
	 * this point and are known to describe an adequate reply message.
	 */
	NET_ASSERT(reader.skipAhead(static_cast<uint16_t>(this->getHeaderSize() + 8)));

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

template<class S, class... Args>
template<class Driver>
inline bool Network<S, Args...>::Ethernet<Driver>::replySent(Launcher *launcher, IoJob* item, Result result)
{
	NET_ASSERT(result == Result::Done);

	state.increment(&DiagnosticCounters::Arp::replySent);
	state.increment(&DiagnosticCounters::Arp::outputSent);

	auto self = static_cast<Ethernet*>(static_cast<ArpReplyJob*>(item));
	self->txReq.template dispose<Pool::Quota::Tx>();

	if(!self->arpRequestQueue.isEmpty()) {
		startReplySequence(launcher, item);
		return true;
	}

	return false;
}

template<class S, class... Args>
template<class Driver>
inline bool Network<S, Args...>::Ethernet<Driver>::assembleReply(Launcher *launcher, IoJob* item, Result result)
{
	NET_ASSERT(result == Result::Done);

	auto self = static_cast<Ethernet*>(static_cast<ArpReplyJob*>(item));
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
        NET_ASSERT(reader.skipAhead(static_cast<uint16_t>(self->getHeaderSize() + 8)));

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
		Route* route = state.routingTable.findRouteWithSource(self, queriedIp);
		if(!route) {
			requestPacket.template dispose<Pool::Quota::Rx>();
			continue;
		} else
			state.routingTable.releaseRoute(route);

		/*
		 * Fill ethernet header with the requester as the destination and write the ARP boilerplate.
		 */
		self->resolver.fillHeader(builder, requesterMac, 0x0806);

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
		NET_ASSERT(builder.copyIn(reinterpret_cast<const char*>(self->resolver.getAddress().bytes), 6) == 6);

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
		launcher->launch(self->getSender(), &Ethernet::replySent, &self->txReq);
        return true;
	}

	builder.done();
	builder.template dispose<Pool::Quota::Tx>();
	return false;
}

template<class S, class... Args>
template<class Driver>
inline bool Network<S, Args...>::Ethernet<Driver>::startReplySequence(Launcher *launcher, IoJob* item)
{
    auto self = static_cast<Ethernet*>(static_cast<ArpReplyJob*>(item));
	static const uint16_t arpReplySize = 28;
	static const uint16_t bufferCount = bytesToBlocks(static_cast<Interface*>(self)->getHeaderSize() + arpReplySize);

	return state.pool.template allocateDirectOrDeferred<Pool::Quota::Tx>(
			launcher,
			&Ethernet::assembleReply,
			&self->poolParams,
			bufferCount);
}

template<class S, class... Args>
template<class Driver>
struct Network<S, Args...>::Ethernet<Driver>::Resolver: ArpTable<Os, Driver::arpCacheEntries, typename Interface::AddressResolver> {
    template<class T> static void callPre(PacketBuilder& packet, decltype(T::preHeaderFill(packet))*) { return T::preHeaderFill(packet); }
    template<class T> static void callPre(...) {}

	virtual const AddressEthernet& getAddress() override final {
		return Driver::ethernetAddress;
	}

	virtual bool resolveAddress(AddressIp4 ip, AddressEthernet& mac) override final {
		return this->lookUp(ip, mac);
	}

	virtual void fillHeader(PacketBuilder& packet, const AddressEthernet& dst, uint16_t etherType) override final
	{
		callPre<Driver>(packet, 0);

		NET_ASSERT(packet.copyIn(reinterpret_cast<const char*>(dst.bytes), 6) == 6);
		NET_ASSERT(packet.copyIn(reinterpret_cast<const char*>(Driver::ethernetAddress.bytes), 6) == 6);
		NET_ASSERT(packet.write16net(etherType));
	}
};

#endif /* ARP_H_ */
