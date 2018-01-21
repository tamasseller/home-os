/*
 * IcmpCore.h
 *
 *  Created on: 2018.01.21.
 *      Author: tooma
 */

#ifndef ICMPCORE_H_
#define ICMPCORE_H_

#include "Network.h"

template<class S, class... Args>
struct Network<S, Args...>::IcmpCore: Network<S, Args...>::RxPacketHandler {
	class EchoReplyJob;
	typedef PacketInputChannel<NullTag> InputChannel;

    EchoReplyJob replyJob;
    InputChannel inputChannel;

	void init() {
		inputChannel.init();
	}

	template<class Reader>
	inline RxPacketHandler* check(Reader& reader)
	{
		InetChecksumDigester sum;
		RxPacketHandler* ret;
		uint16_t typeCode;
		size_t offset = 0;

		state.increment(&DiagnosticCounters::Icmp::inputReceived);

		if(!reader.read16net(typeCode))
			goto formatError;

		switch(typeCode) {
			case 0x0000: // Echo reply.
				ret = this;
				break;
			case 0x0800: // Echo request.
				ret = &replyJob;
				break;
			default:
				goto formatError;
		}

		while(!reader.atEop()) {
			Chunk chunk = reader.getChunk();
			sum.consume(chunk.start, chunk.length, offset & 1);
			offset += chunk.length;
			reader.advance(static_cast<uint16_t>(chunk.length));
		}

		sum.patch(correctEndian(typeCode));

		if(sum.result() != 0)
			goto formatError;

		return ret;

	formatError:

		state.increment(&DiagnosticCounters::Icmp::inputFormatError);
		return nullptr;
	}

	virtual void handlePacket(Packet packet) {
		    PacketStream reader(packet);

			/*
			 * Skip initial fields of the IP header payload all the way to the source address.
			 */
			NET_ASSERT(reader.skipAhead(12));

			AddressIp4 src;
			NET_ASSERT(reader.read32net(src.addr));

			if(!inputChannel.takePacket(packet))
				packet.template dispose<Pool::Quota::Rx>();

	}
};

#endif /* ICMPCORE_H_ */
