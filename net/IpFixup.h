/*
 * IpFixup.h
 *
 *  Created on: 2017.12.05.
 *      Author: tooma
 */

#ifndef IPFIXUP_H_
#define IPFIXUP_H_

template<class S, class... Args>
struct Network<S, Args...>::DummyDigester {
	inline void consume(const char* data, size_t length, bool oddOffset = false) {}
};

template<class S, class... Args>
inline void Network<S, Args...>::fillInitialIpHeader(
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

template<class S, class... Args>
template<bool patch, class HeaderDigester, class PayloadDigester>
inline uint16_t Network<S, Args...>::headerFixupStepOne(
		Packet packet,
		size_t l2headerLength,
		size_t ipHeaderLength,
		HeaderDigester &headerChecksum,
		PayloadDigester &payloadChecksum,
		uint8_t ttl,
		uint8_t protocol)
{
	static constexpr size_t ttlOffset = 8;
	static constexpr size_t protocolOffset = 9;

	PacketDisassembler disassembler;
	disassembler.init(packet);

	size_t length = 0;
	const size_t headerLength = ipHeaderLength;

	/*
	 * Iterate over blocks until the full L2 header is skipped.
	 */
	while(true) {
		Chunk chunk = disassembler.getCurrentChunk();

		/*
		 * Check if the current block is contains the end of the L2 header.
		 */
		if(chunk.length() >= l2headerLength) {
			/*
			 * If the current block has the end of the L2 header in it,
			 * check if it also contains the ttl and protocol fields or
			 * even if it also has the full IP header in it.
			 *
			 * First, initialize the IP packet length to be the part of
			 * this block that is not the L2 header.
			 */
			length = chunk.length() - l2headerLength;

			/*
			 * The _length_ variable effectively holds the offset of the
			 * first byte of the next block relative to the start of the
			 * IP header, and _skip_ is the offset of the IP header
			 * inside this packet:
			 *
			 *  | L2 L2 L2 L2 L2 L2 L2 | -> | L2 L2 L2 IP IP IP IP |
			 *                               <--skip--><--length-->
			 */
			if(patch && (ttlOffset < length))
				chunk.start[l2headerLength + ttlOffset] = ttl;

			if(patch && (protocolOffset < length))
				chunk.start[l2headerLength + protocolOffset] = protocol;

			if(length >= headerLength) {
				/*
				 * If the current block contains only part of the header and also
				 * some payload data then make the header digester consume only
				 * the header part, and the rest goes to the payload digester.
				 */
				headerChecksum.consume(chunk.start + l2headerLength, headerLength);

				const size_t payloadLength = length - headerLength;
				payloadChecksum.consume(chunk.start + l2headerLength + headerLength, payloadLength);
			} else {
				/*
				 * If the rest of the packet is full of header data then
				 * consume all of it.
				 */
				headerChecksum.consume(chunk.start + l2headerLength, length);
			}
			break;
		} else {
			/*
			 * If the current block still contains only a part of the L2 header
			 * account for the skipped amount and go on to the next block.
			 */
			l2headerLength -= chunk.length();
		}

		NET_ASSERT(disassembler.advance());
	}

	/*
	 * If the whole of the header is still not done, then go on
	 * and calculate the checksum over it.
	 */
	while(length < headerLength) {
		NET_ASSERT(disassembler.advance());

		Chunk chunk = disassembler.getCurrentChunk();

		const size_t leftoverHeaderLength = headerLength - length;
		const size_t newLength = length + chunk.length();

		if(patch && (ttlOffset < newLength))
			chunk.start[ttlOffset - length] = ttl;

		if(patch && (protocolOffset < newLength))
			chunk.start[protocolOffset - length] = protocol;

		if(chunk.length() >= leftoverHeaderLength) {
			/*
			 * Again, if the current block contains part of the header,
			 * and also some payload data then process them with their
			 * respective digesters.
			 *
			 * As the _length_ variable  is only updated at the end of
			 * the loop it is now containing the offset of the start of
			 * this chunk relative to the start of the IP content, if it
			 * is odd then bytes are needed to be swapped after calculations,
			 * as indicated by the last argument of these method invocation.
			 */
			headerChecksum.consume(chunk.start, leftoverHeaderLength, length & 1);

			const size_t payloadLength = length - leftoverHeaderLength;
			payloadChecksum.consume(chunk.start + leftoverHeaderLength, payloadLength, payloadLength & 1);
		} else {
			/*
			 * If all is header, then process all if it as that (as above).
			 */
			headerChecksum.consume(chunk.start, chunk.length(), length & 1); // If block < 20byte (LCOV_EXCL_LINE)
		}

		length = newLength;
	};

	/*
	 * If the header is done, go over the rest of the packet to find
	 * the size and calculate the checksum of the payload as well.
	 */
	while(disassembler.advance()) {
		Chunk chunk = disassembler.getCurrentChunk();
		payloadChecksum.consume(chunk.start, chunk.length(), length & 1);
		length += chunk.length();
	};

	headerChecksum.patch(0, correctEndian(static_cast<uint16_t>(length)));

	NET_ASSERT(length <= UINT16_MAX);

	return static_cast<uint16_t>(length);
}

template<class S, class... Args>
inline void Network<S, Args...>::headerFixupStepTwo(
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

#endif /* IPFIXUP_H_ */
