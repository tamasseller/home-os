/*
 * RawReceiver.h
 *
 *  Created on: 2018.01.21.
 *      Author: tooma
 */

#ifndef RAWRECEIVER_H_
#define RAWRECEIVER_H_

#include "Network.h"

template<class S, class... Args>
class Network<S, Args...>::RawReceiver: public Os::template IoRequest<
		IpRxJob<RawReceiver, typename RawCore::InputChannel>,
		&IpRxJob<RawReceiver, typename RawCore::InputChannel>::onBlocking>
{
    friend class RawReceiver::IpRxJob;

    AddressIp4 peerAddress;
    uint8_t protocol;

    inline void reset() {
        protocol = -1;
        peerAddress = AddressIp4::allZero;
    }

    inline void preprocess(Packet) {
        uint8_t ihl;
        NET_ASSERT(this->read8(ihl));
        NET_ASSERT(this->skipAhead(8));
        NET_ASSERT(this->read8(this->protocol));
        NET_ASSERT(this->skipAhead(2));
        NET_ASSERT(this->read32net(this->peerAddress.addr));
        NET_ASSERT(this->skipAhead(static_cast<uint16_t>(((ihl - 4) & 0x0f) << 2)));
    }

public:
    void init() {
        RawReceiver::IpRxJob::init();
        RawReceiver::IoRequest::init();
    }

    AddressIp4 getPeerAddress() const {
        return peerAddress;
    }

    uint8_t getProtocol() const {
        return protocol;
    }

    bool receive() {
        return this->launch(&RawReceiver::IpRxJob::startReception, &state.rawCore.inputChannel);
    }

    void close() {
        this->cancel();
        this->wait();
    }
};

#endif /* RawReceiver_H_ */
