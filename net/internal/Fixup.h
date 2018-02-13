/*
 * Fixup.h
 *
 *  Created on: 2017.12.05.
 *      Author: tooma
 */

#ifndef FIXUP_H_
#define FIXUP_H_

#include "Network.h"
#include "tcp/TcpPacket.h"

template<class S, class... Args>
struct Network<S, Args...>::Fixup {
	static inline void fillInitialIpHeader(
			PacketBuilder &packet,
			AddressIp4 srcIp,
			AddressIp4 dstIp)
	{
	    using namespace IpPacket;

	    StructuredAccessor<Meta, Length, Identification, Fragmentation, Ttl, Protocol, Checksum, SourceAddress, DestinationAddress> accessor;
        accessor.get<Meta>().clear();
        accessor.get<Meta>().setHeaderLength(20);

        accessor.get<Identification>() = 0;

        accessor.get<Fragmentation>().clear();
        accessor.get<Fragmentation>().setDontFragment(true);

        accessor.get<SourceAddress>() = srcIp;
        accessor.get<DestinationAddress>() = dstIp;
        NET_ASSERT(accessor.fill(packet));
	}

	template<class HeaderDigester, class PayloadDigester>
	static inline uint16_t headerFixupStepOne(
			Packet packet,
			size_t l2headerLength,
			HeaderDigester &headerChecksum,
			PayloadDigester &payloadChecksum,
			uint8_t ttl,
			uint8_t protocol)
	{
        using namespace IpPacket;

		SummedPacketStream stream(packet);

		stream.skipAhead(static_cast<uint16_t >(l2headerLength));

        StructuredAccessor<Meta> metaAccessor;
        NET_ASSERT(metaAccessor.extract(stream));

        StructuredAccessor<Length, Ttl, Protocol, Checksum> accessor;
        accessor.get<Length>() = 0;
        accessor.get<Ttl>() = ttl;
        accessor.get<Protocol>() = protocol;
        accessor.get<Checksum>() = 0;
        NET_ASSERT(accessor.fillFrom<Meta>(stream));

		stream.finish();
		headerChecksum.patch(stream.getReducedState());

		stream.start(0xffff);
		stream.finish();
		payloadChecksum.patch(stream.getReducedState());

		auto length = stream.getLength() - l2headerLength;
		headerChecksum.patch(correctEndian(static_cast<uint16_t>(length)));

		stream.goToEnd();
		return static_cast<uint16_t>(length);
	}

	static inline void headerFixupStepTwo(
			PacketStream &modifier,
			size_t l2headerLength,
			uint16_t length,
			uint16_t headerChecksum)
	{
	    using namespace IpPacket;

	    modifier.skipAhead(static_cast<uint16_t >(l2headerLength));

        StructuredAccessor<Length, Checksum> accessor;
        accessor.get<Length>() = static_cast<uint16_t>(length);
        accessor.get<Checksum>() = headerChecksum;
        NET_ASSERT(accessor.fill(modifier));
	}

	static inline void tcpTxPostProcess(PacketStream& stream, InetChecksumDigester& checksum, size_t payload)
	{
		using namespace TcpPacket;

	    checksum.patch(correctEndian(static_cast<uint16_t>(IpProtocolNumbers::tcp)));
		checksum.patch(correctEndian(static_cast<uint16_t>(payload)));

		StructuredAccessor<Checksum> accessor;
		accessor.get<Checksum>() = checksum.result();
		NET_ASSERT(accessor.fill(stream));

		state.increment(&DiagnosticCounters::Tcp::outputQueued);
	}

	template<class Stream, class... Fields>
	static inline auto extractAndSkip(Stream& stream) {
		StructuredAccessor<IpPacket::Meta, Fields...> accessor;

        NET_ASSERT(accessor.extract(stream));

        NET_ASSERT(stream.skipAhead(static_cast<uint16_t>(
            accessor.template get<IpPacket::Meta>().getHeaderLength()
            - (decltype(accessor)::LastField::offset + decltype(accessor)::LastField::length)
        )));

        return accessor;
	}
};

#endif /* IPFIXUP_H_ */
