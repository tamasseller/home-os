/*
 * UdpReceiver.h
 *
 *  Created on: 2018.01.21.
 *      Author: tooma
 */

#ifndef UDPRECEIVER_H_
#define UDPRECEIVER_H_

#include "Network.h"

template<class S, class... Args>
class Network<S, Args...>::UdpReceiver:
    public Os::template IoRequest<
    		IpRxJob<UdpReceiver, typename UdpCore::InputChannel>,
    		&IpRxJob<UdpReceiver, typename UdpCore::InputChannel>::onBlocking>
{
	friend class UdpReceiver::IpRxJob;

	AddressIp4 peerAddress;
    uint16_t peerPort;
    uint16_t length;

    inline void reset() {
        peerAddress = AddressIp4::allZero;
    	peerPort = 0;
    }

    inline void preprocess(Packet) {
        StructuredAccessor<IpPacket::Meta, IpPacket::SourceAddress> ipAccessor;// TODO Move blocks like this into utility functions.
        NET_ASSERT(ipAccessor.extract(*this));

        NET_ASSERT(this->skipAhead(static_cast<uint16_t>(
            ipAccessor.get<IpPacket::Meta>().getHeaderLength()
            - (IpPacket::SourceAddress::offset + IpPacket::SourceAddress::length)
        )));

        StructuredAccessor<UdpPacket::SourcePort, UdpPacket::Length, UdpPacket::End> udpAccessor;
        NET_ASSERT(udpAccessor.extract(*this));

        this->peerAddress = ipAccessor.get<IpPacket::SourceAddress>();
        this->peerPort = udpAccessor.get<UdpPacket::SourcePort>();
        this->length = static_cast<uint16_t>(udpAccessor.get<UdpPacket::Length>() - UdpPacket::End::offset);

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
		return this->launch(&UdpReceiver::IpRxJob::startReception, &state.udpCore.inputChannel);
	}

	void close() {
		this->cancel();
		this->wait();
	}
};

#endif /* UDPRECEIVER_H_ */
