/*
 * Icmp.h
 *
 *  Created on: 2017.12.21.
 *      Author: tooma
 */

#ifndef ICMP_H_
#define ICMP_H_

#include "Network.h"

template<class S, class... Args>
inline void Network<S, Args...>::processIcmpPacket(typename Os::Event*, uintptr_t arg)
{
	Packet chain;
	chain.init(reinterpret_cast<Block*>(arg));
}

template<class S, class... Args>
class Network<S, Args...>::IcmpEchoReplyJob: public IpReplyJob<IcmpEchoReplyJob>
{
	typedef typename IcmpEchoReplyJob::IpReplyJob Base;
	friend Base;

	inline typename Base::InitialReplyInfo getReplyInfo(Packet& request) {
		PacketStream reader;
		reader.init(request);

    	uint8_t headerLength;
    	reader.read8(headerLength);
    	headerLength = static_cast<uint8_t>((headerLength & 0xf) << 2);

		reader.skipAhead(1);	// skip version+ihl and dsp+ecn

		uint16_t totalLength;
		reader.read16net(totalLength);

		reader.skipAhead(8);

		AddressIp4 requesterIp;
		reader.read32net(requesterIp.addr);

    	// TODO check ICMP sum. (type and code is known to be ok)

		return typename Base::InitialReplyInfo{requesterIp, static_cast<uint16_t>(totalLength - headerLength)};
	}

	inline typename Base::FinalReplyInfo generateReply(Packet& request, TxPacketBuilder& reply) {
		PacketStream reader;
		reader.init(request);

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
			reply.write16net(checksum); // type and code

			do {
				Chunk chunk = reader.getChunk();

				bool ok = reply.copyIn(chunk.start, static_cast<uint16_t>(chunk.length()));
				Os::assert(ok, NetErrorStrings::unknown);

				reader.advance(static_cast<uint16_t>(chunk.length()));
			} while(!reader.atEop());
		}

		return typename Base::FinalReplyInfo{0x01 /* ICMP */, 0xff};
	}
};

#endif /* ICMP_H_ */
