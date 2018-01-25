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

	inline typename Base::InitialReplyInfo getReplyInfo(Packet& request)
	{
		PacketStream reader(request);

		StructuredAccessor<IpPacket::Meta, IpPacket::Length, IpPacket::SourceAddress> accessor;

        NET_ASSERT(accessor.extract(reader));

  		state.increment(&DiagnosticCounters::Icmp::pingRequests);

		return typename Base::InitialReplyInfo {
		    accessor.get<IpPacket::SourceAddress>(),
            static_cast<uint16_t>(accessor.get<IpPacket::Meta>().getHeaderLength() + 8 + icmpHeaderSize)
		};
	}

	inline typename Base::FinalReplyInfo generateReply(Packet& request, PacketBuilder& reply)
	{
		PacketStream reader(request);

        reply.write16net(durPurTypeCode);   // type and code
        reply.write16net(~durPurTypeCode);  // checksum
        reply.write32raw(0);                // rest of header

        StructuredAccessor<IpPacket::Meta> accessor;

        NET_ASSERT(accessor.extract(reader));
        NET_ASSERT(reply.write16net(accessor.get<IpPacket::Meta>().data));

    	reply.copyFrom(reader, static_cast<uint16_t>(accessor.get<IpPacket::Meta>().getHeaderLength() + 8 - 2));

		state.increment(&DiagnosticCounters::Icmp::outputQueued);

		return typename Base::FinalReplyInfo{IpProtocolNumbers::icmp, 0xff};
	}
};



#endif /* ICMPDURPURREPLYJOB_H_ */
