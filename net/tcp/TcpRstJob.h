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
        AddressIp4 peerAddress;
        uint8_t ihl;

		PacketStream reader(request);

        NET_ASSERT(reader.read8(ihl));
        NET_ASSERT((ihl & 0xf0) == 0x40);

        NET_ASSERT(reader.skipAhead(11));
        NET_ASSERT(reader.read32net(peerAddress.addr));

		state.increment(&DiagnosticCounters::Tcp::rstPackets);

		return typename Base::InitialReplyInfo{peerAddress, static_cast<uint16_t>(20)};
	}

	inline typename Base::FinalReplyInfo generateReply(Packet& request, PacketBuilder& reply)
	{
		using namespace TcpConstants;
    	uint32_t seqNum, ackNum;
        uint16_t peerPort, localPort, payloadLength, flags;
        uint8_t ihl;

		PacketStream reader(request);

        NET_ASSERT(reader.read8(ihl));
        NET_ASSERT(reader.skipAhead(1));

        NET_ASSERT(reader.read16net(payloadLength));
        payloadLength = static_cast<uint16_t>(payloadLength - ((ihl & 0x0f) << 2));

        NET_ASSERT((ihl & 0xf0) == 0x40); // IPv4 only for now.
        NET_ASSERT(reader.skipAhead(static_cast<uint16_t>((((ihl - 1) & 0x0f) << 2))));

        NET_ASSERT(reader.read16net(peerPort));
        NET_ASSERT(reader.read16net(localPort));
        NET_ASSERT(reader.read32net(seqNum));
        NET_ASSERT(reader.read32net(ackNum));
        NET_ASSERT(reader.read16net(flags));
        payloadLength = static_cast<uint16_t>(payloadLength - ((flags >> 10) & ~0x3));

        if(flags & synMask) payloadLength++;
        if(flags & finMask) payloadLength++;

		NET_ASSERT(reply.write16net(localPort));
		NET_ASSERT(reply.write16net(peerPort));

		NET_ASSERT(reply.write32net((flags & ackMask) ? ackNum : 0));		// seq number
		NET_ASSERT(reply.write32net(seqNum + payloadLength));				// ack number

		NET_ASSERT(reply.write16net((flags & ackMask) ? 0x5004: 0x5014));	// Flags
		NET_ASSERT(reply.write16net(0));		// Window size
		NET_ASSERT(reply.write32net(0));		// Checksum and urgent pointer.

		return typename Base::FinalReplyInfo{IpProtocolNumbers::tcp, 0xff};
	}
};

#endif /* TCPRSTJOB_H_ */
