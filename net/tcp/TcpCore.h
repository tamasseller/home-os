/*
 * TcpCore.h
 *
 *  Created on: 2018.01.21.
 *      Author: tooma
 */

#ifndef TCPCORE_H_
#define TCPCORE_H_

#include "Network.h"

#include "TcpConstants.h"

template<class S, class... Args>
struct Network<S, Args...>::TcpCore: Network<S, Args...>::RxPacketHandler
{
	typedef PacketInputChannel<DstPortTag> ListenerChannel;
	class InputChannel;
	class RetransmitJob;
	class RstJob;

	RetransmitJob retransmitJob;
	RstJob rstJob;
	ListenerChannel listenerChannel;
    InputChannel inputChannel;

    void init() {
        inputChannel.init();
        listenerChannel.init();
    }

	template<class Reader>
	inline RxPacketHandler* check(Reader& reader, uint16_t length)
	{
		using namespace TcpConstants;

		uint16_t offsetAndFlags, payloadLength;
		uint8_t offset;

		state.increment(&DiagnosticCounters::Tcp::inputReceived);

		if(length < 20)
			goto formatError;

		if(!reader.skipAhead(12) || !reader.read16net(offsetAndFlags))
			goto formatError;

		offset = static_cast<uint8_t>((offsetAndFlags >> 10) & ~0x3);

		if(length < offset)
			goto formatError;

		reader.patch(correctEndian(static_cast<uint16_t>(length)));
		reader.patch(correctEndian(static_cast<uint16_t>(IpProtocolNumbers::tcp)));

		if(!reader.finish() || reader.result())
			goto formatError;

		return this;

	formatError:

		state.increment(&DiagnosticCounters::Tcp::inputFormatError);
		return nullptr;
	}

	virtual void handlePacket(Packet packet)
	{
		using namespace TcpConstants;
	    PacketStream reader(packet);

		uint8_t ihl;
		uint16_t peerPort, localPort;
		AddressIp4 peerAddress;
		NET_ASSERT(reader.read8(ihl));
		NET_ASSERT((ihl & 0xf0) == 0x40);

		NET_ASSERT(reader.skipAhead(11));
		NET_ASSERT(reader.read32net(peerAddress.addr));

		NET_ASSERT(reader.skipAhead(static_cast<uint16_t>(((ihl - 4) & 0x0f) << 2)));

		NET_ASSERT(reader.read16net(peerPort));
		NET_ASSERT(reader.read16net(localPort));

		NET_ASSERT(reader.skipAhead(8));

		uint16_t offsetAndFlags;
		NET_ASSERT(reader.read16net(offsetAndFlags));

		if(!inputChannel.takePacket(packet, ConnectionTag(peerAddress, peerPort, localPort))) {
			if((offsetAndFlags & rstMask)) {
				packet.template dispose<Pool::Quota::Rx>();
			} else {
				bool isInitial = (offsetAndFlags & synMask) && !(offsetAndFlags & ackMask);
				if(!isInitial || !listenerChannel.takePacket(packet, DstPortTag(localPort))) {
					state.increment(&DiagnosticCounters::Tcp::inputNoPort);
					rstJob.handlePacket(packet);
				}
			}
		}
	}
};

#endif /* TCPCORE_H_ */
