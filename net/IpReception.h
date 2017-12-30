/*
 * IpReception.h
 *
 *  Created on: 2017.12.30.
 *      Author: tooma
 */

#ifndef IPRECEPTION_H_
#define IPRECEPTION_H_

template<class S, class... Args>
template<class Reader>
inline bool Network<S, Args...>::checkIcmpPacket(Reader& reader) {
		uint16_t typeCode;
		if(!reader.read16net(typeCode) ||
				(typeCode != 0x0800)) { // Echo request.
			// TODO add the other allowed ICMP message type+codes.

			return false;
		}

		InetChecksumDigester sum;
		size_t offset = 0;
		while(!reader.atEop()) {
			Chunk chunk = reader.getChunk();
			sum.consume(chunk.start, chunk.length(), offset & 1);
			offset += chunk.length();
			reader.advance(static_cast<uint16_t>(chunk.length()));
		}

		sum.patch(0, correctEndian(typeCode));
		return sum.result() == 0;
}

template<class S, class... Args>
inline void Network<S, Args...>::ipPacketReceived(Packet packet, Interface* dev) {
	struct ChecksumValidatorObserver: InetChecksumDigester {
		size_t remainingHeaderLength;
		size_t totalLength;
		bool valid;

		bool isDone() {
			return !remainingHeaderLength;
		}

		void observeInternalBlock(char* start, char* end) {
			size_t length = end - start;
			totalLength += length;

			if(length < remainingHeaderLength) {
				this->consume(start, length, remainingHeaderLength & 1);
				remainingHeaderLength -= length;
			} else if(remainingHeaderLength) {
				this->consume(start, remainingHeaderLength, remainingHeaderLength & 1);
				remainingHeaderLength = 0;
				valid = this->result() == 0;
			}

		}

		void observeFirstBlock(char* start, char* end) {
			remainingHeaderLength = (start[0] & 0xf) << 2;
			totalLength = 0;
			observeInternalBlock(start, end);
		}
	};

	/*
	 * Dispose of L2 header.
	 */
	packet.template dropInitialBytes<Pool::Quota::Rx>(dev->getHeaderSize());

	ObservedPacketStream<ChecksumValidatorObserver> reader;
	reader.init(packet);

	uint8_t versionAndHeaderLength;
	if(!reader.read8(versionAndHeaderLength) || ((versionAndHeaderLength & 0xf0) != 0x40)) {
		packet.template dispose<Pool::Quota::Rx>();
		return;
	}

	if(!reader.skipAhead(1)) {
		packet.template dispose<Pool::Quota::Rx>();
		return;
	}

	uint16_t ipLength;
	if(!reader.read16net(ipLength)) {
		packet.template dispose<Pool::Quota::Rx>();
		return;
	}

	uint16_t id;
	if(!reader.read16net(id)) {
		packet.template dispose<Pool::Quota::Rx>();
		return;
	}

	uint16_t fragmentOffsetAndFlags;
	if(!reader.read16net(fragmentOffsetAndFlags)) {
		packet.template dispose<Pool::Quota::Rx>();
		return;
	}

	static constexpr uint16_t offsetAndMoreFragsMask = 0x3fff;
	if(fragmentOffsetAndFlags & offsetAndMoreFragsMask) {
		/*
		 * IP defragmentation could be done here.
		 */
		packet.template dispose<Pool::Quota::Rx>();
		return;
	}

	/*
	 * Skip TTL.
	 */
	if(!reader.skipAhead(1)) {
		packet.template dispose<Pool::Quota::Rx>();
		return;
	}

	uint8_t protocol;
	if(!reader.read8(protocol)
			|| (protocol != 1)) { // TODO add the other protocols.

		// TODO send ICMP unreachable message.
		packet.template dispose<Pool::Quota::Rx>();
		return;
	}

	if(!reader.skipAhead(6)) {
		packet.template dispose<Pool::Quota::Rx>();
		return;
	}

	AddressIp4 dstIp;
	if(!reader.read32net(dstIp.addr)) {
		packet.template dispose<Pool::Quota::Rx>();
		return;
	}

	if(!reader.skipAhead(static_cast<uint16_t>(reader.remainingHeaderLength))) {
		packet.template dispose<Pool::Quota::Rx>();
		return;
	}

	Os::assert(reader.isDone(), NetErrorStrings::unknown);
	if(!reader.valid) {
		packet.template dispose<Pool::Quota::Rx>();
		return;
	}

	/*
	 * Check if the packet is for us.
	 */
	if(Route* route = state.routingTable.findRouteWithSource(dev, dstIp)) {
		state.routingTable.releaseRoute(route);
	} else {
		/*
		 * If this host not the final destination then, routing could be done here.
		 */
		packet.template dispose<Pool::Quota::Rx>();
		return;
	}

	if((protocol == 1) && !checkIcmpPacket(reader)) {
		packet.template dispose<Pool::Quota::Rx>();
		return;
	}

	bool infinitePacket = reader.skipAhead(0xffff);
	Os::assert(!infinitePacket, NetErrorStrings::unknown);

	if(reader.totalLength != ipLength) {
		packet.template dispose<Pool::Quota::Rx>();
		return;
	}

	switch(protocol) {
	case 1: // ICMP
		state.icmpReplyJob.arrangeReply(packet);
		break;
	}
}

#endif /* IPRECEPTION_H_ */
