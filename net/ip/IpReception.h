/*
 * IpReception.h
 *
 *  Created on: 2017.12.30.
 *      Author: tooma
 */

#ifndef IPRECEPTION_H_
#define IPRECEPTION_H_

#include "Network.h"

template<class S, class... Args>
inline void Network<S, Args...>::ipPacketReceived(Packet packet, Interface* dev)
{
    /*
     * Dispose of L2 header.
     */

    packet.template dropInitialBytes<Pool::Quota::Rx>(dev->getHeaderSize()); // TODO check success

    state.increment(&DiagnosticCounters::Ip::inputReceived);

    PacketStream reader(packet);

    uint8_t versionAndHeaderLength;
    if(!reader.read8(versionAndHeaderLength))
        goto formatError;
    if((versionAndHeaderLength & 0xf0) == 0x40) {
        if(!reader.skipAhead(15))
            goto formatError;

        AddressIp4 dstIp;
        if(!reader.read32net(dstIp.addr))
            goto formatError;

        /*
         * Check if the packet is for us.
         */
        if(Route* route = state.routingTable.findRouteWithSource(dev, dstIp)) {
            state.routingTable.releaseRoute(route);
        } else {
            // TODO allow broadcast.

            /*
             * If this host is not the final destination then, routing could be done here.
             */
            state.increment(&DiagnosticCounters::Ip::inputForwardingRequired);
            goto error;
        }

        state.rxProcessor.process(packet);
    } else {
        // IPv4 only for now.
        goto formatError;
    }

    return;

    formatError:
        state.increment(&DiagnosticCounters::Ip::inputFormatError);

    error:
        packet.template dispose<Pool::Quota::Rx>();
}

template<class S, class... Args>
inline void Network<S, Args...>::processReceivedPacket(Packet packet)
{
    static constexpr uint16_t offsetAndMoreFragsMask = 0x3fff;

    RxPacketHandler* handler = &state.rawCore;
    AddressIp4 srcIp, dstIp;
    uint16_t ipLength, id, fragmentOffsetAndFlags, ret, ihl;
    uint8_t protocol;

    SummedPacketStream reader(packet);

    NET_ASSERT(reader.read16net(ret));

    NET_ASSERT((ret & 0xf000) == 0x4000); // IPv4 only for now.
    ihl = static_cast<uint16_t>((ret & 0x0f00) >> 6);

    if(ihl < 20)
            goto formatError;

    reader.start(ihl - 2);
    reader.patch(correctEndian(ret));

    NET_ASSERT(reader.read16net(ipLength));
    NET_ASSERT(reader.read16net(id));
	NET_ASSERT(reader.read16net(fragmentOffsetAndFlags));

    if(fragmentOffsetAndFlags & offsetAndMoreFragsMask) {
        /*
         * IP defragmentation could be done here.
         */
        state.increment(&DiagnosticCounters::Ip::inputReassemblyRequired);
        goto error;
    }

    /*
     * Skip TTL.
     */
    NET_ASSERT(reader.skipAhead(1));
    NET_ASSERT(reader.read8(protocol));
    NET_ASSERT(reader.skipAhead(2));
    NET_ASSERT(reader.read32net(srcIp.addr) && reader.read32net(dstIp.addr));

	if(!reader.finish() || reader.result())
        goto formatError;

    switch(protocol) {
    case IpProtocolNumbers::icmp:
        handler = state.icmpCore.check(reader);
        break;
    case IpProtocolNumbers::tcp:
    case IpProtocolNumbers::udp: {
            auto payloadLength = static_cast<uint16_t>(ipLength - ihl);
            reader.start(payloadLength);
            reader.patch(correctEndian(static_cast<uint16_t>(srcIp.addr >> 16)));
            reader.patch(correctEndian(static_cast<uint16_t>(srcIp.addr & 0xffff)));
            reader.patch(correctEndian(static_cast<uint16_t>(dstIp.addr >> 16)));
            reader.patch(correctEndian(static_cast<uint16_t>(dstIp.addr & 0xffff)));

            if(protocol == 6) {
                handler = state.tcpCore.check(reader, payloadLength);
            } else {
                handler = state.udpCore.check(reader, payloadLength);
            }

            break;
        }
    }

    if(handler) {
    	if(!reader.goToEnd() || (reader.getLength() != ipLength))
            goto formatError;

        handler->handlePacket(packet);
        return;
    }

formatError:
    state.increment(&DiagnosticCounters::Ip::inputFormatError);

error:
    packet.template dispose<Pool::Quota::Rx>();
}

#endif /* IPRECEPTION_H_ */
