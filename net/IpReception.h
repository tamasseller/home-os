/*
 * IpReception.h
 *
 *  Created on: 2017.12.30.
 *      Author: tooma
 */

#ifndef IPRECEPTION_H_
#define IPRECEPTION_H_

template<class S, class... Args>
inline void Network<S, Args...>::processRawPacket(Packet start)
{
	if(!state.rawInputChannel.takePacket(start))
		start.template dispose<Pool::Quota::Rx>();
}

template<class S, class... Args>
inline void Network<S, Args...>::ipPacketReceived(Packet packet, Interface* dev)
{
    /*
     * Dispose of L2 header.
     */

    packet.template dropInitialBytes<Pool::Quota::Rx>(dev->getHeaderSize()); // TODO check success

    state.increment(&DiagnosticCounters::Ip::inputReceived);

    PacketStream reader(packet);

    uint8_t versionAndHeaderLength;
    if(!reader.read8(versionAndHeaderLength))
        goto formatError;
    if((versionAndHeaderLength & 0xf0) == 0x40) {
        if(!reader.skipAhead(15))
            goto formatError;

        AddressIp4 dstIp;
        if(!reader.read32net(dstIp.addr))
            goto formatError;

        /*
         * Check if the packet is for us.
         */
        if(Route* route = state.routingTable.findRouteWithSource(dev, dstIp)) {
            state.routingTable.releaseRoute(route);
        } else {
            // TODO allow broadcast.

            /*
             * If this host is not the final destination then, routing could be done here.
             */
            state.increment(&DiagnosticCounters::Ip::inputForwardingRequired);
            goto error;
        }

        state.rxProcessor.process(packet);
    } else {
        // IPv4 only for now.
        goto formatError;
    }

    return;

    formatError:
        state.increment(&DiagnosticCounters::Ip::inputFormatError);

    error:
        packet.template dispose<Pool::Quota::Rx>();
}

template<class S, class... Args>
inline void Network<S, Args...>::processReceivedPacket(Packet packet)
{
    static constexpr uint16_t offsetAndMoreFragsMask = 0x3fff;

    static struct RawPacketHandler: RxPacketHandler {
        virtual void handlePacket(Packet packet) {
            processRawPacket(packet);
        }
    } rawPacketHandler;

    RxPacketHandler* handler = &rawPacketHandler;
    AddressIp4 srcIp, dstIp;
    uint16_t ipLength, id, fragmentOffsetAndFlags, ret, ihl;
    uint8_t protocol;

    SummedPacketStream reader(packet);

    NET_ASSERT(reader.read16net(ret));

    NET_ASSERT((ret & 0xf000) == 0x4000); // IPv4 only for now.
    ihl = static_cast<uint16_t>((ret & 0x0f00) >> 6);

    if(ihl < 20)
            goto formatError;

    reader.start(ihl - 2);
    reader.patch(correctEndian(ret));

    NET_ASSERT(reader.read16net(ipLength));
    NET_ASSERT(reader.read16net(id));
	NET_ASSERT(reader.read16net(fragmentOffsetAndFlags));

    if(fragmentOffsetAndFlags & offsetAndMoreFragsMask) {
        /*
         * IP defragmentation could be done here.
         */
        state.increment(&DiagnosticCounters::Ip::inputReassemblyRequired);
        goto error;
    }

    /*
     * Skip TTL.
     */
    NET_ASSERT(reader.skipAhead(1));
    NET_ASSERT(reader.read8(protocol));
    NET_ASSERT(reader.skipAhead(2));
    NET_ASSERT(reader.read32net(srcIp.addr) && reader.read32net(dstIp.addr));

	if(!reader.finish() || reader.result())
        goto formatError;

    switch(protocol) {
    case IpProtocolNumbers::icmp:
        handler = checkIcmpPacket(reader);
        break;
    case IpProtocolNumbers::tcp:
    case IpProtocolNumbers::udp: {
            auto payloadLength = static_cast<uint16_t>(ipLength - ihl);
            reader.start(payloadLength);
            reader.patch(correctEndian(static_cast<uint16_t>(srcIp.addr >> 16)));
            reader.patch(correctEndian(static_cast<uint16_t>(srcIp.addr & 0xffff)));
            reader.patch(correctEndian(static_cast<uint16_t>(dstIp.addr >> 16)));
            reader.patch(correctEndian(static_cast<uint16_t>(dstIp.addr & 0xffff)));

            if(protocol == 6) {
                handler = checkTcpPacket(reader, payloadLength);
            } else {
                handler = checkUdpPacket(reader, payloadLength);
            }

            break;
        }
    }

    if(handler) {
    	if(!reader.goToEnd() || (reader.getLength() != ipLength))
            goto formatError;

        handler->handlePacket(packet);
        return;
    }

formatError:
    state.increment(&DiagnosticCounters::Ip::inputFormatError);

error:
    packet.template dispose<Pool::Quota::Rx>();
}


template<class S, class... Args>
template<class Child, class Channel>
class Network<S, Args...>::IpRxJob: public Os::IoJob, public PacketStream {
protected:
    Packet packet; // TODO check if can be peeked from data.packets instead.

    typename Channel::IoData data;

    inline void preprocess() {}
    inline void reset() {}

    inline void invalidateAllStates() {
        packet.init(nullptr);
        static_cast<PacketStream*>(this)->invalidate();
        static_cast<Child*>(this)->reset();
    }

private:
    inline bool fetchPacket()
    {
        if(packet.isValid()) {
        	/*
        	 * If we have a valid packet that is not consumed return true
        	 * right away to indicate that there is data to be processed.
        	 */
        	if(!this->atEop())
        		return true;

        	/*
        	 * Drop the packet if it have been consumed.
        	 */
        	packet.template dispose<Pool::Quota::Rx>();
			invalidateAllStates();
        }

    	/*
    	 * If there is no packet (because either there was none or it have
    	 * been dropped above) and a new one can be obtained then fetch it
    	 * and return true to indicate that there is data to be processed.
    	 */
        if(!packet.isValid() && data.packets.take(packet)) {
			static_cast<PacketStream*>(this)->init(packet);
			static_cast<Child*>(this)->preprocess();
			return true;
        }

        /*
         * If this point is reached then there is no data to be read.
         */
        return false;
    }

    static bool received(Launcher *launcher, IoJob* item, Result result)
    {
        auto self = static_cast<IpRxJob*>(item);

        if(result == Result::Done) {
            self->fetchPacket();
            return true;
        } else {
            if(self->packet.isValid())
                self->packet.template dispose<Pool::Quota::Rx>();

            while(self->data.packets.take(self->packet)) // TODO optimize chain deallocation at once.
                self->packet.template dispose<Pool::Quota::Rx>();

            self->invalidateAllStates();
            return false;
        }
    }

public:
    // Internal!
    bool onBlocking()
    {
        return !fetchPacket();
    }


    void init() {
        invalidateAllStates();
    }

    static bool startReception(Launcher *launcher, IoJob* item, Channel* channel)
    {
        auto self = static_cast<IpRxJob*>(item);

        NET_ASSERT(!self->packet.isValid());

        launcher->launch(channel, &IpRxJob::received, &self->data);
        return true;
    }
};

template<class S, class... Args>
class Network<S, Args...>::IpReceiver: public Os::template IoRequest<IpRxJob<IpReceiver, RawInputChannel>, &IpRxJob<IpReceiver, RawInputChannel>::onBlocking>
{
    friend class IpReceiver::IpRxJob;

    AddressIp4 peerAddress;
    uint8_t protocol;

    inline void reset() {
        protocol = -1;
        peerAddress = AddressIp4::allZero;
    }

    inline void preprocess() {
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
        IpReceiver::IpRxJob::init();
        IpReceiver::IoRequest::init();
    }

    AddressIp4 getPeerAddress() const {
        return peerAddress;
    }

    uint8_t getProtocol() const {
        return protocol;
    }

    bool receive() {
        return this->launch(&IpReceiver::IpRxJob::startReception, &state.rawInputChannel);
    }

    void close() {
        this->cancel();
        this->wait();
    }
};

#endif /* IPRECEPTION_H_ */
