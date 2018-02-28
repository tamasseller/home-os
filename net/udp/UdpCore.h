/*
 * UdpCore.h
 *
 *  Created on: 2018.01.21.
 *      Author: tooma
 */

#ifndef UDPCORE_H_
#define UDPCORE_H_

#include "Network.h"
#include "UdpPacket.h"

template<class S, class... Args>
struct Network<S, Args...>::UdpCore: Network<S, Args...>::RxPacketHandler {
	typedef PacketInputChannel<DstPortTag> InputChannel;
	InputChannel inputChannel;

	void init() {
	    inputChannel.init();
	}

	template<class Reader>
	inline RxPacketHandler* check(Reader& reader, uint16_t length)
	{
		state.increment(&DiagnosticCounters::Udp::inputReceived);

		if(length < 8)
			goto formatError;

		StructuredAccessor<UdpPacket::Checksum> accessor;

		if(!accessor.extract(reader))
			goto formatError;

		/*
		 * RFC768 User Datagram Protocol (https://tools.ietf.org/html/rfc768) says:
		 *
		 * 		"An all zero transmitted checksum value means that the transmitter generated
		 * 		no checksum  (for debugging or for higher level  protocols that don't care)."
		 */
		if(accessor.get<UdpPacket::Checksum>() != 0) {
			reader.patch(home::reverseBytes16(static_cast<uint16_t>(length)));
			reader.patch(home::reverseBytes16(static_cast<uint16_t>(IpProtocolNumbers::udp)));

			if(!reader.finish() || reader.result())
				goto formatError;
		}

		return this;

	formatError:

		state.increment(&DiagnosticCounters::Udp::inputFormatError);
		return nullptr;
	}

	virtual void handlePacket(Packet packet)
	{
	    PacketStream reader(packet);

		StructuredAccessor<IpPacket::Meta> ipAccessor;

		NET_ASSERT(ipAccessor.extract(reader));
		NET_ASSERT(reader.skipAhead(static_cast<uint16_t>(ipAccessor.get<IpPacket::Meta>().getHeaderLength() - 2)));

		StructuredAccessor<UdpPacket::DestinationPort> udpAccessor;

		NET_ASSERT(udpAccessor.extract(reader));

		if(!inputChannel.takePacket(packet, DstPortTag(udpAccessor.get<UdpPacket::DestinationPort>()))) {
            state.increment(&DiagnosticCounters::Udp::inputNoPort);
		    state.icmpCore.durPurJob.handlePacket(packet);
		}
	}
};

#endif /* UDPCORE_H_ */
