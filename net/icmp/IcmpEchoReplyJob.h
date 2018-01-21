/*
 * IcmpEchoReplyJob.h
 *
 *  Created on: 2018.01.21.
 *      Author: tooma
 */

#ifndef ICMPECHOREPLYJOB_H_
#define ICMPECHOREPLYJOB_H_

#include "Network.h"

template<class S, class... Args>
class Network<S, Args...>::IcmpCore::EchoReplyJob: public IpReplyJob<EchoReplyJob>
{
	typedef typename EchoReplyJob::IpReplyJob Base;
	friend Base;

	inline void replySent() {
		state.increment(&DiagnosticCounters::Icmp::outputSent);
	}

	inline typename Base::InitialReplyInfo getReplyInfo(Packet& request) {
		PacketStream reader(request);

    	uint8_t headerLength;
    	NET_ASSERT(reader.read8(headerLength));
    	headerLength = static_cast<uint8_t>((headerLength & 0xf) << 2);

    	NET_ASSERT(reader.skipAhead(1));	// skip version+ihl and dsp+ecn

		uint16_t totalLength;
		NET_ASSERT(reader.read16net(totalLength));

		NET_ASSERT(reader.skipAhead(8));

		AddressIp4 requesterIp;
		NET_ASSERT(reader.read32net(requesterIp.addr));

		state.increment(&DiagnosticCounters ::Icmp::pingRequests);

		return typename Base::InitialReplyInfo{requesterIp, static_cast<uint16_t>(totalLength - headerLength)};
	}

	inline typename Base::FinalReplyInfo generateReply(Packet& request, PacketBuilder& reply) {
		PacketStream reader(request);

    	uint8_t headerLength;
    	reader.read8(headerLength);
    	headerLength = static_cast<uint8_t>((headerLength & 0xf) << 2);
    	reader.skipAhead(static_cast<uint16_t>(headerLength - 1));

		uint16_t typeCode;
		reader.read16net(typeCode);

		if(typeCode == 0x800) {
			uint16_t checksum;
			reader.read16net(checksum);
			uint32_t temp = checksum + 0x800;
			temp = (temp & 0xffff) + (temp >> 16);
			checksum = static_cast<uint16_t>((temp & 0xffff) + (temp >> 16));

			reply.write16net(0); // type and code
			reply.write16net(checksum);

			do {
				Chunk chunk = reader.getChunk();

				NET_ASSERT(reply.copyIn(chunk.start, static_cast<uint16_t>(chunk.length)));

				reader.advance(static_cast<uint16_t>(chunk.length));
			} while(!reader.atEop());
		}

		state.increment(&DiagnosticCounters::Icmp::outputQueued);

		return typename Base::FinalReplyInfo{IpProtocolNumbers::icmp, 0xff};
	}
};

#endif /* ICMPECHOREPLYJOB_H_ */
