/*
 * TcpCore.h
 *
 *  Created on: 2018.01.21.
 *      Author: tooma
 */

#ifndef TCPCORE_H_
#define TCPCORE_H_

#include "Network.h"

#include "TcpPacket.h"

template<class S, class... Args>
struct Network<S, Args...>::TcpCore: Network<S, Args...>::RxPacketHandler
{
	typedef PacketInputChannel<DstPortTag> ListenerChannel;
	class InputChannel;
	class RetransmitJob;
	class AckJob;
	class RstJob;

	template<class> class TcpTx;
	template<class> class TcpRxJob;
	template<class> class TcpRx;

	RetransmitJob retransmitJob;
	AckJob ackJob;
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
        using namespace TcpPacket;

		uint16_t offsetAndFlags, payloadLength;
		uint8_t offset;

		state.increment(&DiagnosticCounters::Tcp::inputReceived);

		if(length < 20)
			goto formatError;

        StructuredAccessor<Flags> tcpAccessor;
        if(!tcpAccessor.extract(reader) || (length < tcpAccessor.get<Flags>().getDataOffset()))
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
		using namespace TcpPacket;
	    PacketStream reader(packet);

    	auto ipAccessor = Fixup::template extractAndSkip<PacketStream, IpPacket::SourceAddress>(reader);

    	StructuredAccessor<SourcePort, DestinationPort, Flags> tcpAccessor;
        NET_ASSERT(tcpAccessor.extract(reader));

		if(!inputChannel.takePacket(packet, ConnectionTag(ipAccessor.template get<IpPacket::SourceAddress>(), tcpAccessor.get<SourcePort>(), tcpAccessor.get<DestinationPort>()))) {
			if(tcpAccessor.get<Flags>().hasRst()) {
				packet.template dispose<Pool::Quota::Rx>();
			} else {
				bool isInitial = tcpAccessor.get<Flags>().hasSyn() && !tcpAccessor.get<Flags>().hasAck();
				if(!isInitial || !listenerChannel.takePacket(packet, DstPortTag(tcpAccessor.get<DestinationPort>()))) {
					state.increment(&DiagnosticCounters::Tcp::inputNoPort);
					rstJob.handlePacket(packet);
				}
			}
		}
	}
};

#endif /* TCPCORE_H_ */
