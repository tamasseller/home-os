/*
 * Udp.h
 *
 *  Created on: 2018.01.02.
 *      Author: tooma
 */

#ifndef UDP_H_
#define UDP_H_

template<class S, class... Args>
template<class Reader>
inline typename Network<S, Args...>::RxPacketHandler* Network<S, Args...>::checkUdpPacket(Reader& reader, uint16_t length)
{
	RxPacketHandler* ret = &state.udpPacketProcessor;

	if(length < 8)
		return nullptr;

	size_t offset = 0;

	if(!reader.skipAhead(6))
		return nullptr;

	uint16_t cheksumField;

	if(!reader.read16net(cheksumField))
		return nullptr;

	/*
	 * RFC768 User Datagram Protocol (https://tools.ietf.org/html/rfc768) says:
	 *
	 * 		"An all zero transmitted checksum value means that the transmitter generated
	 * 		no checksum  (for debugging or for higher level  protocols that don't care)."
	 */
	if(cheksumField) {
		Os::assert(!reader.skipAhead(0xffff), NetErrorStrings::unknown);

		reader.patch(0, correctEndian(static_cast<uint16_t>(length)));
		reader.patch(0, correctEndian(static_cast<uint16_t>(0x11)));

		if(reader.result())
			return nullptr;
	}

	return ret;
}

template<class S, class... Args>
struct Network<S, Args...>::UdpPacketProcessor: PacketProcessor, RxPacketHandler {
    inline UdpPacketProcessor(): PacketProcessor(&Network<S, Args...>::processUdpPacket) {}

private:
    virtual void handlePacket(Packet packet) {
        this->process(packet);
    }
};

template<class S, class... Args>
inline void Network<S, Args...>::processUdpPacket(typename Os::Event*, uintptr_t arg)
{
    Packet chain;
    chain.init(reinterpret_cast<Block*>(arg));

    PacketStream reader; // TODO optimize with PacketDisassembler
    reader.init(chain);

    while(true) {
        Packet start = reader.asPacket();

        uint8_t ihl;

        Os::assert(reader.read8(ihl), NetErrorStrings::unknown);

        Os::assert(reader.skipAhead(static_cast<uint16_t>(((ihl & 0x0f) << 2) + 1)), NetErrorStrings::unknown);

        uint16_t port;
        Os::assert(reader.read16net(port), NetErrorStrings::unknown);

        bool hasMore = reader.cutCurrentAndMoveToNext();

        if(!state.udpInputChannel.takePacket(start, DstPortTag(port)))
            start.template dispose<Pool::Quota::Rx>(); // TODO reply ICMP DUR PUR

        if(!hasMore)
            break;
    }
}

template<class S, class... Args>
class Network<S, Args...>::UdpReceiver:
    public Os::template IoRequest<IpRxJob<UdpReceiver, UdpInputChannel>, &IpRxJob<UdpReceiver, UdpInputChannel>::onBlocking>
{
	friend class UdpReceiver::IpRxJob;

	AddressIp4 peerAddress;
    uint16_t peerPort;

    inline void reset() {
        peerAddress = AddressIp4::allZero;
    	peerPort = 0;
    }

    inline void preprocess() {
        uint8_t ihl;
        Os::assert(this->read8(ihl), NetErrorStrings::unknown);

        Os::assert(this->skipAhead(11), NetErrorStrings::unknown);
        Os::assert(this->read32net(this->peerAddress.addr), NetErrorStrings::unknown);

        Os::assert(this->skipAhead(static_cast<uint16_t>(((ihl - 4) & 0x0f) << 2)), NetErrorStrings::unknown);

        Os::assert(this->read16net(this->peerPort), NetErrorStrings::unknown);

        Os::assert(this->skipAhead(6), NetErrorStrings::unknown);
    }

public:
    AddressIp4 getPeerAddress() const {
        return peerAddress;
    }

    uint16_t getPeerPort() const {
        return peerPort;
    }

	void init() {
	    UdpReceiver::IpRxJob::init();
		UdpReceiver::IoRequest::init();
	}

	bool receive(uint16_t port) {
		this->data.accessTag() = DstPortTag(port);
		return this->launch(&UdpReceiver::IpRxJob::startReception, &state.udpInputChannel);
	}

	void close() {
		this->cancel();
		this->wait();
	}
};

template<class S, class... Args>
class Network<S, Args...>::UdpTransmitter: public IpTransmitterBase<UdpTransmitter> {
	friend class UdpTransmitter::IpTransmitterBase::IpTxJob;

	uint16_t dstPort, srcPort;

	static inline bool onPreparationDone(Launcher *launcher, IoJob* item) {
		auto self = static_cast<UdpTransmitter*>(item);
		Os::assert(self->accessPacket().write16net(self->srcPort), NetErrorStrings::unknown);
		Os::assert(self->accessPacket().write16net(self->dstPort), NetErrorStrings::unknown);
		Os::assert(self->accessPacket().write32raw(0), NetErrorStrings::unknown);

		return false;
	}

	inline void postProcess(PacketStream& stream, InetChecksumDigester& checksum, size_t payload) {
		Os::assert(stream.skipAhead(4), NetErrorStrings::unknown);
		stream.write16net(static_cast<uint16_t>(payload));

		checksum.patch(0, correctEndian(static_cast<uint16_t>(0x11)));
		checksum.patch(0, correctEndian(static_cast<uint16_t>(payload)));
		checksum.patch(0, correctEndian(static_cast<uint16_t>(payload)));

		uint16_t result = checksum.result();

		/*
		 * RFC768 User Datagram Protocol (https://tools.ietf.org/html/rfc768) says:
		 *
		 * 		"If the computed checksum is zero, it is transmitted as all
		 * 		 ones (the equivalent in one's complement arithmetic). An all
		 * 		 zero transmitted checksum value means that the transmitter
		 * 		 generated  no checksum  (for debugging or for higher level
		 * 		 protocols that don't care)."
		 */
		Os::assert(stream.write16raw(result ? result : static_cast<uint16_t>(~result)), NetErrorStrings::unknown);
	}
public:

	inline void setSourcePort(uint16_t srcPort) {
		this->srcPort = srcPort;
	}

	inline void init(uint16_t srcPort) {
		this->srcPort = srcPort;
		UdpTransmitter::IpTransmitterBase::init();
	}

	inline bool prepare(AddressIp4 dst, uint16_t dstPort, size_t inLineSize, size_t indirectCount = 0)
	{
    	this->reset();
    	inLineSize += 8; // UDP header size;
    	this->dstPort = dstPort;
		bool later = this->launch(&UdpTransmitter::IpTransmitterBase::IpTxJob::startPreparation, dst, inLineSize, indirectCount, arpRequestRetry);
		return later || this->getError() == nullptr;
	}

    inline bool prepareTimeout(size_t timeout, AddressIp4 dst, size_t inLineSize, size_t indirectCount = 0)
    {
    	this->reset();
    	inLineSize += 8; // UDP header size;
    	this->dstPort = dstPort;
        bool later = this->launchTimeout(&UdpTransmitter::IpTransmitterBase::IpTxJob::startPreparation, timeout, dst, inLineSize, indirectCount, arpRequestRetry);
        return later || this->getError() == nullptr;
    }

	inline bool send(uint8_t ttl = 64)
	{
		if(!this->check())
			return false;

		bool later = this->launch(
				&UdpTransmitter::IpTransmitterBase::IpTxJob::template startTransmission<InetChecksumDigester>,
				'\x11', ttl);
        return later || this->getError() == nullptr;
	}

    inline bool sendTimeout(size_t timeout, uint8_t ttl = 64)
    {
        if(!this->check())
            return false;

        bool later = this->launchTimeout(
        		&UdpTransmitter::IpTransmitterBase::IpTxJob::template startTransmission<InetChecksumDigester>, timeout,
        		'\x11', ttl);
        return later || this->getError() == nullptr;
    }
};


#endif /* UDP_H_ */
