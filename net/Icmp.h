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
template<class Reader>
inline typename Network<S, Args...>::RxPacketHandler* Network<S, Args...>::checkIcmpPacket(Reader& reader)
{
    static struct IcmpPacketHandler: RxPacketHandler {
        virtual void handlePacket(Packet packet) {
            processIcmpPacket(packet);
        }
    } icmpPacketHandler;

	InetChecksumDigester sum;
	RxPacketHandler* ret;
	uint16_t typeCode;
	size_t offset = 0;

	state.increment(&DiagnosticCounters::Icmp::inputReceived);

	if(!reader.read16net(typeCode))
		goto formatError;

	switch(typeCode) {
		case 0x0000: // Echo reply.
			ret = &icmpPacketHandler;
			break;
		case 0x0800: // Echo request.
			ret = &state.icmpReplyJob;
			break;
		default:
			goto formatError;
	}

	while(!reader.atEop()) {
		Chunk chunk = reader.getChunk();
		sum.consume(chunk.start, chunk.length(), offset & 1);
		offset += chunk.length();
		reader.advance(static_cast<uint16_t>(chunk.length()));
	}

	sum.patch(0, correctEndian(typeCode));

	if(sum.result() != 0)
		goto formatError;

	return ret;

formatError:

	state.increment(&DiagnosticCounters::Icmp::inputFormatError);
	return nullptr;
}

template<class S, class... Args>
inline void Network<S, Args...>::processIcmpPacket(Packet start)
{
    PacketStream reader;
    reader.init(start);

	/*
	 * Skip initial fields of the IP header payload all the way to the source address.
	 */
	NET_ASSERT(reader.skipAhead(12));

	AddressIp4 src;
	NET_ASSERT(reader.read32net(src.addr));

	if(!state.icmpInputChannel.takePacket(start))
		start.template dispose<Pool::Quota::Rx>();
}

template<class S, class... Args>
class Network<S, Args...>::IcmpEchoReplyJob: public IpReplyJob<IcmpEchoReplyJob>
{
	typedef typename IcmpEchoReplyJob::IpReplyJob Base;
	friend Base;

	inline void replySent(Packet& request) {
		state.increment(&DiagnosticCounters::Icmp::outputSent);
	}

	inline typename Base::InitialReplyInfo getReplyInfo(Packet& request) {
		PacketStream reader;
		reader.init(request);

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
			reply.write16net(checksum);

			do {
				Chunk chunk = reader.getChunk();

				NET_ASSERT(reply.copyIn(chunk.start, static_cast<uint16_t>(chunk.length())));

				reader.advance(static_cast<uint16_t>(chunk.length()));
			} while(!reader.atEop());
		}

		state.increment(&DiagnosticCounters::Icmp::outputQueued);

		return typename Base::FinalReplyInfo{IpProtocolNumbers::icmp, 0xff};
	}
};

template<class S, class... Args>
class Network<S, Args...>::IcmpReceiver:
    public Os::template IoRequest<IpRxJob<IcmpReceiver, IcmpInputChannel>, &IpRxJob<IcmpReceiver, IcmpInputChannel>::onBlocking>
{
	friend typename IcmpReceiver::IoRequest::IpRxJob;

    inline void preprocess() {
    	state.increment(&DiagnosticCounters::Icmp::inputProcessed);
    }

public:
	void init() {
	    IcmpReceiver::IpRxJob::init();
		IcmpReceiver::IoRequest::init();
	}

	bool receive() {
		return this->launch(&IcmpReceiver::IpRxJob::startReception, &state.icmpInputChannel);
	}

	void close() {
		this->cancel();
		this->wait();
	}
};

#endif /* ICMP_H_ */
