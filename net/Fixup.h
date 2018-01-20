/*
 * Fixup.h
 *
 *  Created on: 2017.12.05.
 *      Author: tooma
 */

#ifndef FIXUP_H_
#define FIXUP_H_

#include "Network.h"

template<class S, class... Args>
struct Network<S, Args...>::Fixup {
	static inline void fillInitialIpHeader(
			PacketBuilder &packet,
			AddressIp4 srcIp,
			AddressIp4 dstIp)
	{
		static constexpr const char initialIpPreamble[12] = {
				0x45,			// ver + ihl
				0x00,			// dscp + ecn
				0x00, 0x00,		// length
				0x00, 0x00,		// fragment identification
				0x40, 0x00,		// flags + fragment offset
				0x00,			// time-to-live
				0x00,			// protocol
				0x00, 0x00		// checksum
		};

		NET_ASSERT(packet.copyIn(initialIpPreamble, sizeof(initialIpPreamble)));
		NET_ASSERT(packet.write32net(srcIp.addr));
		NET_ASSERT(packet.write32net(dstIp.addr));
	}

	template<class HeaderDigester, class PayloadDigester>
	static inline uint16_t headerFixupStepOne(
			Packet packet,
			size_t l2headerLength,
			size_t ipHeaderLength,
			HeaderDigester &headerChecksum,
			PayloadDigester &payloadChecksum,
			uint8_t ttl,
			uint8_t protocol)
	{
		static constexpr size_t ttlOffset = 8;

		ValidatorPacketStream reader(packet, static_cast<uint16_t >(l2headerLength));

		reader.skipAhead(static_cast<uint16_t >(ttlOffset));
		reader.finish();
		headerChecksum.patch(reader.getReducedState());

		reader.restart(0xffff);
		reader.goToEnd();
		payloadChecksum.patch(reader.getReducedState());

		headerChecksum.patch(correctEndian(static_cast<uint16_t>(reader.getLength())));

		return static_cast<uint16_t>(reader.getLength());
	}

	static inline void headerFixupStepTwo(
			PacketStream &modifier,
			size_t l2HeaderSize,
			uint16_t length,
			uint16_t headerChecksum)
	{
		static constexpr size_t lengthOffset = 2;
		static constexpr size_t skipBetweenLengthAndChecksum = 6;

		NET_ASSERT(modifier.skipAhead(static_cast<uint16_t>(l2HeaderSize + lengthOffset)));
		NET_ASSERT(modifier.write16net(static_cast<uint16_t>(length)));
		NET_ASSERT(modifier.skipAhead(skipBetweenLengthAndChecksum));
		NET_ASSERT(modifier.write16raw(headerChecksum));
	}

	static inline void tcpTxPostProcess(PacketStream& stream, InetChecksumDigester& checksum, size_t payload)
	{
		checksum.patch(correctEndian(static_cast<uint16_t>(IpProtocolNumbers::tcp)));
		checksum.patch(correctEndian(static_cast<uint16_t>(payload)));

		NET_ASSERT(stream.skipAhead(16));
		NET_ASSERT(stream.write16raw(checksum.result()));

		state.increment(&DiagnosticCounters::Tcp::outputQueued);
	}
};

#endif /* IPFIXUP_H_ */
