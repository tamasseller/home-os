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

        StructuredAccessor<SourcePort, DestinationPort, SequenceNumber, AcknowledgementNumber, Flags> tcpAccessor;

        NET_ASSERT(tcpAccessor.extract(reader));

        auto payloadLength = static_cast<uint16_t>(ipAccessor.get<IpPacket::Length>()
                          - (ipAccessor.get<IpPacket::Meta>().getHeaderLength() + tcpAccessor.get<Flags>().getDataOffset()));

        if(tcpAccessor.get<Flags>().hasSyn()) payloadLength++;
        if(tcpAccessor.get<Flags>().hasFin()) payloadLength++;

		NET_ASSERT(reply.write16net(tcpAccessor.get<DestinationPort>()));
		NET_ASSERT(reply.write16net(tcpAccessor.get<SourcePort>()));

		NET_ASSERT(reply.write32net(tcpAccessor.get<Flags>().hasAck() ? tcpAccessor.get<AcknowledgementNumber>() : 0));		// seq number
		NET_ASSERT(reply.write32net(tcpAccessor.get<SequenceNumber>() + payloadLength));				// ack number

		NET_ASSERT(reply.write16net(tcpAccessor.get<Flags>().hasAck() ? 0x5004: 0x5014));	// Flags
		NET_ASSERT(reply.write16net(0));		// Window size
		NET_ASSERT(reply.write32net(0));		// Checksum and urgent pointer.

		return typename Base::FinalReplyInfo{IpProtocolNumbers::tcp, 0xff};
	}
};

#endif /* TCPRSTJOB_H_ */
