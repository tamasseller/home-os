/*
 * IpReception.h
 *
 *  Created on: 2017.12.30.
 *      Author: tooma
 */

#ifndef IPRECEPTION_H_
#define IPRECEPTION_H_

template<class S, class... Args>
struct Network<S, Args...>::RawPacketProcessor: PacketProcessor, RxPacketHandler {
    inline RawPacketProcessor(): PacketProcessor(&Network<S, Args...>::processRawPacket) {}

private:
    virtual void handlePacket(Packet packet) {
        this->process(packet);
    }
};

template<class S, class... Args>
inline void Network<S, Args...>::processRawPacket(typename Os::Event*, uintptr_t arg)
{
    Packet chain;
    chain.init(reinterpret_cast<Block*>(arg));

    PacketStream reader; // TODO optimize with PacketDisassembler
    reader.init(chain);

    while(true) {
        Packet start = reader.asPacket();
        bool hasMore = reader.cutCurrentAndMoveToNext();

        if(!state.rawInputChannel.takePacket(start))
            start.template dispose<Pool::Quota::Rx>();

        if(!hasMore)
            break;
    }
}

template<class S, class... Args>
template<class Reader>
inline typename Network<S, Args...>::RxPacketHandler* Network<S, Args...>::checkTcpPacket(Reader& reader)
{
	RxPacketHandler* ret = nullptr;
	return ret;
}

template<class S, class... Args>
inline void Network<S, Args...>::ipPacketReceived(Packet packet, Interface* dev) {
	struct ChecksumValidatorObserver: InetChecksumDigester {
		size_t remainingHeaderLength;
		size_t totalLength;
		bool valid;

		bool isDone() {
			return !remainingHeaderLength;
		}

		void observeInternalBlock(char* start, char* end) {
			size_t length = end - start;
			totalLength += length;

			if(length < remainingHeaderLength) {
				this->consume(start, length, remainingHeaderLength & 1);
				remainingHeaderLength -= length;
			} else if(remainingHeaderLength) {
				this->consume(start, remainingHeaderLength, remainingHeaderLength & 1);
				remainingHeaderLength = 0;
				valid = this->result() == 0;
			}

		}

		void observeFirstBlock(char* start, char* end) {
			remainingHeaderLength = (start[0] & 0xf) << 2;
			totalLength = 0;
			observeInternalBlock(start, end);
		}
	};

	/*
	 * Dispose of L2 header.
	 */
	packet.template dropInitialBytes<Pool::Quota::Rx>(dev->getHeaderSize());

	ObservedPacketStream<ChecksumValidatorObserver> reader;
	reader.init(packet);

	uint8_t versionAndHeaderLength;
	if(!reader.read8(versionAndHeaderLength) || ((versionAndHeaderLength & 0xf0) != 0x40)) {
		packet.template dispose<Pool::Quota::Rx>();
		return;
	}

	if(!reader.skipAhead(1)) {
		packet.template dispose<Pool::Quota::Rx>();
		return;
	}

	uint16_t ipLength;
	if(!reader.read16net(ipLength)) {
		packet.template dispose<Pool::Quota::Rx>();
		return;
	}

	uint16_t id;
	if(!reader.read16net(id)) {
		packet.template dispose<Pool::Quota::Rx>();
		return;
	}

	uint16_t fragmentOffsetAndFlags;
	if(!reader.read16net(fragmentOffsetAndFlags)) {
		packet.template dispose<Pool::Quota::Rx>();
		return;
	}

	static constexpr uint16_t offsetAndMoreFragsMask = 0x3fff;
	if(fragmentOffsetAndFlags & offsetAndMoreFragsMask) {
		/*
		 * IP defragmentation could be done here.
		 */
		packet.template dispose<Pool::Quota::Rx>();
		return;
	}

	/*
	 * Skip TTL.
	 */
	if(!reader.skipAhead(1)) {
		packet.template dispose<Pool::Quota::Rx>();
		return;
	}

	uint8_t protocol;
	if(!reader.read8(protocol)) {
		packet.template dispose<Pool::Quota::Rx>();
		return;
	}

	if(!reader.skipAhead(2)) {
		packet.template dispose<Pool::Quota::Rx>();
		return;
	}

	AddressIp4 srcIp, dstIp;
	if(!reader.read32net(srcIp.addr) || !reader.read32net(dstIp.addr)) {
		packet.template dispose<Pool::Quota::Rx>();
		return;
	}

	if(!reader.skipAhead(static_cast<uint16_t>(reader.remainingHeaderLength))) {
		packet.template dispose<Pool::Quota::Rx>();
		return;
	}

	Os::assert(reader.isDone(), NetErrorStrings::unknown);
	if(!reader.valid) {
		packet.template dispose<Pool::Quota::Rx>();
		return;
	}

	/*
	 * Check if the packet is for us.
	 */
	if(Route* route = state.routingTable.findRouteWithSource(dev, dstIp)) {
		state.routingTable.releaseRoute(route);
	} else {
		/*
		 * If this host not the final destination then, routing could be done here.
		 */
		packet.template dispose<Pool::Quota::Rx>();
		return;
	}

	RxPacketHandler* handler = &state.rawPacketProcessor;

	switch(protocol) {
	case 1:
		handler = checkIcmpPacket(reader);
		break;
	case 6:
		handler = checkTcpPacket(reader);
		break;
	case 17:
		handler = checkUdpPacket(reader);
		break;
	}

    bool infinitePacket = reader.skipAhead(0xffff);
    Os::assert(!infinitePacket, NetErrorStrings::unknown);

    if(reader.totalLength != ipLength) {
        packet.template dispose<Pool::Quota::Rx>();
        return;
    }

    if(handler)
        handler->handlePacket(packet);
    else
        packet.template dispose<Pool::Quota::Rx>();
}

template<class S, class... Args>
template<class Child, class Channel>
class Network<S, Args...>::IpRxJob: public Os::IoJob, public PacketStream {
    Packet packet;

protected:
    typename Channel::IoData data;

    inline void preprocess() {}
    inline void reset() {}

private:
    inline void invalidateAllStates() {
        packet.init(nullptr);
        static_cast<PacketStream*>(this)->invalidate();
        static_cast<Child*>(this)->reset();
    }

    inline void fetchPacket() {
        if(packet.isValid() && this->atEop()) {
            packet.template dispose<Pool::Quota::Rx>();
            invalidateAllStates();
        }

        if(!packet.isValid()) {
            if(data.packets.takePacketFromQueue(packet)) {
                static_cast<PacketStream*>(this)->init(packet);
                static_cast<Child*>(this)->preprocess();
            }
        }
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

            while(self->data.packets.takePacketFromQueue(self->packet))
                self->packet.template dispose<Pool::Quota::Rx>();

            self->invalidateAllStates();
            return false;
        }
    }

public:
    // Internal!
    bool onBlocking()
    {
        if(isEmpty())
            return true;

        fetchPacket();
        return false;
    }


    void init() {
        invalidateAllStates();
    }

    bool isEmpty() {
        return !packet.isValid() && data.packets.isEmpty();
    }

    static bool startReception(Launcher *launcher, IoJob* item, Channel* channel)
    {
        auto self = static_cast<IpRxJob*>(item);

        Os::assert(!self->packet.isValid(), NetErrorStrings::unknown);

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
        Os::assert(this->read8(ihl), NetErrorStrings::unknown);

        Os::assert(this->skipAhead(8), NetErrorStrings::unknown);
        Os::assert(this->read8(this->protocol), NetErrorStrings::unknown);

        Os::assert(this->skipAhead(2), NetErrorStrings::unknown);
        Os::assert(this->read32net(this->peerAddress.addr), NetErrorStrings::unknown);

        Os::assert(this->skipAhead(static_cast<uint16_t>(((ihl - 4) & 0x0f) << 2)), NetErrorStrings::unknown);
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
