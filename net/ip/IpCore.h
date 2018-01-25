/*
 * IpCore.h
 *
 *  Created on: 2018.01.23.
 *      Author: tooma
 */

#ifndef IPCORE_H_
#define IPCORE_H_

#include "Network.h"

#include "IpPacket.h"

template<class S, class... Args>
class Network<S, Args...>::IpCore: PacketProcessor {
	friend PacketProcessor;

	inline void processReceivedPacket(Packet packet)
	{
		using namespace IpPacket;

	    static constexpr uint16_t offsetAndMoreFragsMask = 0x3fff;

	    RxPacketHandler* handler = &state.rawCore;
	    AddressIp4 srcIp, dstIp;
	    uint16_t ipLength, id, fragmentOffsetAndFlags, ret, ihl;
	    uint8_t protocol;

	    SummedPacketStream reader(packet);

	    StructuredAccessor<Meta, Length, Fragmentation, Protocol, SourceAddress, DestinationAddress> accessor;

	    NET_ASSERT(accessor.extract(reader));

	    if(accessor.get<Fragmentation>().isFragmented()) {
	        /*
	         * IP defragmentation could be done here.
	         */
	        state.increment(&DiagnosticCounters::Ip::inputReassemblyRequired);
	        goto error;
	    }

		if(!reader.finish() || reader.result())
	        goto formatError;

	    switch(accessor.get<Protocol>()) {
	    case IpProtocolNumbers::icmp:
	        handler = state.icmpCore.check(reader);
	        break;
	    case IpProtocolNumbers::tcp:
	    case IpProtocolNumbers::udp: {
	            auto payloadLength = static_cast<uint16_t>(accessor.get<Length>() - accessor.get<Meta>().getHeaderLength());
	            reader.start(payloadLength);
	            reader.patchNet(static_cast<uint16_t>(accessor.get<SourceAddress>().addr >> 16));
	            reader.patchNet(static_cast<uint16_t>(accessor.get<SourceAddress>().addr & 0xffff));
	            reader.patchNet(static_cast<uint16_t>(accessor.get<DestinationAddress>().addr >> 16));
	            reader.patchNet(static_cast<uint16_t>(accessor.get<DestinationAddress>().addr & 0xffff));

	            if(accessor.get<Protocol>() == 6) {
	                handler = state.tcpCore.check(reader, payloadLength);
	            } else {
	                handler = state.udpCore.check(reader, payloadLength);
	            }

	            break;
	        }
	    }

	    if(handler) {
	    	if(!reader.goToEnd() || (reader.getLength() != accessor.get<Length>()))
	            goto formatError;

	        handler->handlePacket(packet);
	        return;
	    }

	formatError:
	    state.increment(&DiagnosticCounters::Ip::inputFormatError);

	error:
	    packet.template dispose<Pool::Quota::Rx>();
	}

public:
	inline void ipPacketReceived(Packet packet, Interface* dev)
	{
		using namespace IpPacket;

	    if(packet.template dropInitialBytes<Pool::Quota::Rx>(dev->getHeaderSize())) {
			state.increment(&DiagnosticCounters::Ip::inputReceived);

			PacketStream reader(packet);

			StructuredAccessor<Meta, DestinationAddress> accessor;

			if(!accessor.extract(reader))
				goto formatError;

			if(Route* route = state.routingTable.findRouteWithSource(dev, accessor.get<DestinationAddress>())) {
				state.routingTable.releaseRoute(route);
			} else {
				// TODO allow broadcast.

				/*
				 * If this host is not the final destination then, routing could be done here.
				 */
				state.increment(&DiagnosticCounters::Ip::inputForwardingRequired);
				goto error;
			}

			this->process(packet);

			return;
	    }

	    formatError:
	        state.increment(&DiagnosticCounters::Ip::inputFormatError);

	    error:
	        packet.template dispose<Pool::Quota::Rx>();
	}

	inline IpCore(): PacketProcessor(PacketProcessor::template make<IpCore, &IpCore::processReceivedPacket>()) {}
};

#endif /* IPCORE_H_ */
