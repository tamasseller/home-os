/*
 * TcpRstJob.h
 *
 *  Created on: 2018.01.21.
 *      Author: tooma
 */

#ifndef TCPRSTJOB_H_
#define TCPRSTJOB_H_

#include "Network.h"

template<class S, class... Args>
class Network<S, Args...>::TcpCore::RstJob: public IpReplyJob<RstJob, InetChecksumDigester>
{
	typedef typename RstJob::IpReplyJob Base;
	friend typename Base::IpTxJob;
	friend Base;

	inline void replySent() {
		state.increment(&DiagnosticCounters::Tcp::outputSent);
	}

	inline void postProcess(PacketStream& stream, InetChecksumDigester& checksum, size_t payload) {
		Fixup::tcpTxPostProcess(stream, checksum, payload);
	}

	inline typename Base::InitialReplyInfo getReplyInfo(Packet& request)
	{
        PacketStream reader(request);

        StructuredAccessor<IpPacket::SourceAddress> ipAccessor;
        NET_ASSERT(ipAccessor.extract(reader));

		state.increment(&DiagnosticCounters::Tcp::rstPackets);

		return typename Base::InitialReplyInfo{ipAccessor.get<IpPacket::SourceAddress>(), static_cast<uint16_t>(20)};
	}

	inline typename Base::FinalReplyInfo generateReply(Packet& request, PacketBuilder& reply)
	{
		using namespace TcpPacket;

        PacketStream reader(request);

        StructuredAccessor<IpPacket::Meta, IpPacket::Length> ipAccessor;         // TODO Move blocks like this into utility functions.
        NET_ASSERT(ipAccessor.extract(reader));

        NET_ASSERT(reader.skipAhead(static_cast<uint16_t>(
            ipAccessor.get<IpPacket::Meta>().getHeaderLength()
            - (IpPacket::Length::offset + IpPacket::Length::length)
        )));

        StructuredAccessor<SourcePort, DestinationPort, SequenceNumber, AcknowledgementNumber, Flags> requestAccessor;

        NET_ASSERT(requestAccessor.extract(reader));

        auto payloadLength = static_cast<uint16_t>(ipAccessor.get<IpPacket::Length>()
                          - (ipAccessor.get<IpPacket::Meta>().getHeaderLength() + requestAccessor.get<Flags>().getDataOffset()));

        if(requestAccessor.get<Flags>().hasSyn()) payloadLength++;
        if(requestAccessor.get<Flags>().hasFin()) payloadLength++;

        StructuredAccessor<SourcePort, DestinationPort, SequenceNumber, AcknowledgementNumber, Flags, WindowSize, Checksum, UrgentPointer> replyAccessor;
        replyAccessor.get<SourcePort>() =               requestAccessor.get<DestinationPort>();
        replyAccessor.get<DestinationPort>() =          requestAccessor.get<SourcePort>();
        replyAccessor.get<SequenceNumber>() =           requestAccessor.get<Flags>().hasAck() ? requestAccessor.get<AcknowledgementNumber>() : 0;
        replyAccessor.get<AcknowledgementNumber>() =    requestAccessor.get<SequenceNumber>() + payloadLength;
        replyAccessor.get<WindowSize>() =               0;
        replyAccessor.get<Checksum>() =                 0;
        replyAccessor.get<UrgentPointer>() =            0;

        replyAccessor.get<Flags>().clear();
        replyAccessor.get<Flags>().setDataOffset(20);
        replyAccessor.get<Flags>().setRst(true);
        replyAccessor.get<Flags>().setAck(!requestAccessor.get<Flags>().hasAck());

        NET_ASSERT(replyAccessor.fill(reply));

		return typename Base::FinalReplyInfo{IpProtocolNumbers::tcp, 0xff};
	}
};

#endif /* TCPRSTJOB_H_ */
