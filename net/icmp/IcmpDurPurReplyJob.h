/*
 * IcmpDurPurReplyJob.h
 *
 *  Created on: 2018.01.23.
 *      Author: tooma
 */

#ifndef ICMPDURPURREPLYJOB_H_
#define ICMPDURPURREPLYJOB_H_

template<class S, class... Args>
class Network<S, Args...>::IcmpCore::DurPurReplyJob: public IpReplyJob<DurPurReplyJob>
{
	typedef typename DurPurReplyJob::IpReplyJob Base;
	using DurPurReplyJob::IpReplyJob::IpTxJob::ipHeaderSize;
	friend Base;

	static constexpr auto icmpHeaderSize = 8;

	inline void replySent() {
		state.increment(&DiagnosticCounters::Icmp::outputSent);
	}

	inline typename Base::InitialReplyInfo getReplyInfo(Packet& request) {
		PacketStream reader(request);

    	uint8_t headerLength;
    	NET_ASSERT(reader.read8(headerLength));
    	headerLength = static_cast<uint8_t>((headerLength & 0xf) << 2);

		NET_ASSERT(reader.skipAhead(11));

		AddressIp4 requesterIp;
		NET_ASSERT(reader.read32net(requesterIp.addr));

		state.increment(&DiagnosticCounters ::Icmp::pingRequests);

		auto length = headerLength + 8 + icmpHeaderSize;

		return typename Base::InitialReplyInfo{requesterIp, static_cast<uint16_t>(length)};
	}

	inline typename Base::FinalReplyInfo generateReply(Packet& request, PacketBuilder& reply) {
		PacketStream reader(request);

		static constexpr auto icmpDurPurTypeCode = 0x0303;

        reply.write16net(icmpDurPurTypeCode);   // type and code
        reply.write16net(~icmpDurPurTypeCode);  // checksum
        reply.write32raw(0);                    // rest of header

    	uint8_t headerLength;
    	reader.read8(headerLength);
    	reply.write8(headerLength);

    	headerLength = static_cast<uint8_t>((headerLength & 0xf) << 2);

    	reply.copyFrom(reader, static_cast<uint16_t>(headerLength + 8 - 1));

		state.increment(&DiagnosticCounters::Icmp::outputQueued);

		return typename Base::FinalReplyInfo{IpProtocolNumbers::icmp, 0xff};
	}
};



#endif /* ICMPDURPURREPLYJOB_H_ */
