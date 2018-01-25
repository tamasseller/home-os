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
        StructuredAccessor<IpPacket::Meta, IpPacket::Protocol, IpPacket::SourceAddress> accessor;
        NET_ASSERT(accessor.extract(*this));

        this->protocol = accessor.get<IpPacket::Protocol>();
        this->peerAddress = accessor.get<IpPacket::SourceAddress>();

        NET_ASSERT(this->skipAhead(static_cast<uint16_t>(
            accessor.get<IpPacket::Meta>().getHeaderLength()
            - (IpPacket::SourceAddress::offset + IpPacket::SourceAddress::length)
        )));
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
