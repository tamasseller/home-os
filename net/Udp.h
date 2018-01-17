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
	uint16_t cheksumField;

	state.increment(&DiagnosticCounters::Udp::inputReceived);

	if(length < 8)
		goto formatError;

	if(!reader.skipAhead(6))
		goto formatError;

	if(!reader.read16net(cheksumField))
		goto formatError;

	/*
	 * RFC768 User Datagram Protocol (https://tools.ietf.org/html/rfc768) says:
	 *
	 * 		"An all zero transmitted checksum value means that the transmitter generated
	 * 		no checksum  (for debugging or for higher level  protocols that don't care)."
	 */
	if(cheksumField) {
		NET_ASSERT(!reader.skipAhead(0xffff));

		reader.patch(0, correctEndian(static_cast<uint16_t>(length)));
		reader.patch(0, correctEndian(static_cast<uint16_t>(IpProtocolNumbers::udp)));

		if(reader.result())
			goto formatError;
	}

	return &state.udpPacketProcessor;

formatError:

	state.increment(&DiagnosticCounters::Udp::inputFormatError);
	return nullptr;
}

template<class S, class... Args>
struct Network<S, Args...>::UdpPacketProcessor: PacketProcessor, RxPacketHandler {
    inline UdpPacketProcessor():
    		PacketProcessor(PacketProcessor::template makeStatic<&Network<S, Args...>::processUdpPacket>()) {}

private:
    virtual void handlePacket(Packet packet) {
        this->process(packet);
    }
};

template<class S, class... Args>
inline void Network<S, Args...>::processUdpPacket(Packet start)
{
    PacketStream reader;
    reader.init(start);

	uint8_t ihl;

	NET_ASSERT(reader.read8(ihl));

	NET_ASSERT(reader.skipAhead(static_cast<uint16_t>(((ihl & 0x0f) << 2) + 1)));

	uint16_t port;
	NET_ASSERT(reader.read16net(port));

	if(!state.udpInputChannel.takePacket(start, DstPortTag(port))) {
		start.template dispose<Pool::Quota::Rx>(); // TODO reply ICMP DUR PUR
		state.increment(&DiagnosticCounters::Udp::inputNoPort);
	}
}

template<class S, class... Args>
class Network<S, Args...>::UdpReceiver:
    public Os::template IoRequest<IpRxJob<UdpReceiver, UdpInputChannel>, &IpRxJob<UdpReceiver, UdpInputChannel>::onBlocking>
{
	friend class UdpReceiver::IpRxJob;

	AddressIp4 peerAddress;
    uint16_t peerPort;
    uint16_t length;

    inline void reset() {
        peerAddress = AddressIp4::allZero;
    	peerPort = 0;
    }

    inline void preprocess() {
        uint8_t ihl;
        NET_ASSERT(this->read8(ihl));

        NET_ASSERT(this->skipAhead(11));
        NET_ASSERT(this->read32net(this->peerAddress.addr));

        NET_ASSERT(this->skipAhead(static_cast<uint16_t>(((ihl - 4) & 0x0f) << 2)));

        NET_ASSERT(this->read16net(this->peerPort));

        NET_ASSERT(this->skipAhead(2));

        NET_ASSERT(this->read16net(this->length));
        this->length = static_cast<uint16_t>(this->length - 8);

        NET_ASSERT(this->skipAhead(2));

        state.increment(&DiagnosticCounters::Udp::inputProcessed);
    }

public:
    AddressIp4 getPeerAddress() const {
        return peerAddress;
    }

    uint16_t getPeerPort() const {
        return peerPort;
    }

    uint16_t getLength() const {
        return length;
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

	inline bool onSent(Launcher *launcher, IoJob* item, Result result) {
		state.increment(&DiagnosticCounters::Udp::outputSent);
		return this->UdpTransmitter::IpTransmitterBase::onSent(launcher, item, result);
	}

	static inline bool onPreparationDone(Launcher *launcher, IoJob* item) {
		auto self = static_cast<UdpTransmitter*>(item);
		NET_ASSERT(self->accessPacket().write16net(self->srcPort));
		NET_ASSERT(self->accessPacket().write16net(self->dstPort));
		NET_ASSERT(self->accessPacket().write32raw(0));

		return false;
	}

	inline void postProcess(PacketStream& stream, InetChecksumDigester& checksum, size_t payload) {
		NET_ASSERT(stream.skipAhead(4));
		stream.write16net(static_cast<uint16_t>(payload));

		checksum.patch(0, correctEndian(static_cast<uint16_t>(IpProtocolNumbers::udp)));
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
		NET_ASSERT(stream.write16raw(result ? result : 0xffff));

		state.increment(&DiagnosticCounters::Udp::outputQueued);
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
				IpProtocolNumbers::udp, ttl);
        return later || this->getError() == nullptr;
	}

    inline bool sendTimeout(size_t timeout, uint8_t ttl = 64)
    {
        if(!this->check())
            return false;

        bool later = this->launchTimeout(
        		&UdpTransmitter::IpTransmitterBase::IpTxJob::template startTransmission<InetChecksumDigester>, timeout,
        		IpProtocolNumbers::udp, ttl);
        return later || this->getError() == nullptr;
    }
};


#endif /* UDP_H_ */
