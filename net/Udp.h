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
inline typename Network<S, Args...>::RxPacketHandler* Network<S, Args...>::checkUdpPacket(Reader& reader)
{
	RxPacketHandler* ret = &state.udpPacketProcessor;
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


#endif /* UDP_H_ */
