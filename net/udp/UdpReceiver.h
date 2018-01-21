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
		return this->launch(&UdpReceiver::IpRxJob::startReception, &state.udpCore.inputChannel);
	}

	void close() {
		this->cancel();
		this->wait();
	}
};

#endif /* UDPRECEIVER_H_ */
