/*
 * UdpCore.h
 *
 *  Created on: 2018.01.21.
 *      Author: tooma
 */

#ifndef UDPCORE_H_
#define UDPCORE_H_

#include "Network.h"

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
		uint16_t cheksumField;

		state.increment(&DiagnosticCounters::Udp::inputReceived);

		if(length < 8)
			goto formatError;

		if(!reader.skipAhead(6))
			goto formatError;

		if(!reader.read16net(cheksumField))
			goto formatError;

		/*
		 * RFC768 User Datagram Protocol (https://tools.ietf.org/html/rfc768) says:
		 *
		 * 		"An all zero transmitted checksum value means that the transmitter generated
		 * 		no checksum  (for debugging or for higher level  protocols that don't care)."
		 */
		if(cheksumField) {
			reader.patch(correctEndian(static_cast<uint16_t>(length)));
			reader.patch(correctEndian(static_cast<uint16_t>(IpProtocolNumbers::udp)));

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

		uint8_t ihl;

		NET_ASSERT(reader.read8(ihl));

		NET_ASSERT(reader.skipAhead(static_cast<uint16_t>(((ihl & 0x0f) << 2) + 1)));

		uint16_t port;
		NET_ASSERT(reader.read16net(port));

		if(!inputChannel.takePacket(packet, DstPortTag(port))) {
			packet.template dispose<Pool::Quota::Rx>(); // TODO reply ICMP DUR PUR
			state.increment(&DiagnosticCounters::Udp::inputNoPort);
		}
	}
};

#endif /* UDPCORE_H_ */
