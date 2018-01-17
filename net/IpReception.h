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
    inline RawPacketProcessor():
    		PacketProcessor(PacketProcessor::template makeStatic<&Network<S, Args...>::processRawPacket>()) {}

private:
    virtual void handlePacket(Packet packet) {
        this->process(packet);
    }
};

template<class S, class... Args>
inline void Network<S, Args...>::processRawPacket(Packet start)
{
	if(!state.rawInputChannel.takePacket(start))
		start.template dispose<Pool::Quota::Rx>();
}

template<class S, class... Args>
inline void Network<S, Args...>::ipPacketReceived(Packet packet, Interface* dev)
{
	struct ChecksumValidatorObserver: InetChecksumDigester {
		size_t remainingLength;
		size_t totalLength;
		bool valid;
		bool offsetOdd;

		bool isDone() {
			return !remainingLength;
		}

		void observeInternalBlock(char* start, char* end) {
			size_t length = end - start;
			totalLength += length;

			if(length < remainingLength) {
				this->consume(start, length, offsetOdd);
				remainingLength -= length;
			} else if(remainingLength) {
				this->consume(start, remainingLength, offsetOdd);
				remainingLength = 0;
				valid = this->result() == 0;
			}

			if(length & 1)
				offsetOdd = !offsetOdd;
		}

		void observeFirstBlock(char* start, char* end) {
			remainingLength = (start[0] & 0xf) << 2;
			totalLength = 0;
			offsetOdd = false;
			observeInternalBlock(start, end);
		}
	};

	static constexpr uint16_t offsetAndMoreFragsMask = 0x3fff;

	RxPacketHandler* handler = &state.rawPacketProcessor;
	AddressIp4 srcIp, dstIp;
	uint16_t ipLength, id, fragmentOffsetAndFlags;
	uint8_t protocol;

	state.increment(&DiagnosticCounters::Ip::inputReceived);

	/*
	 * Dispose of L2 header.
	 */
	packet.template dropInitialBytes<Pool::Quota::Rx>(dev->getHeaderSize());

	ObservedPacketStream<ChecksumValidatorObserver> reader;
	reader.init(packet);

	uint8_t versionAndHeaderLength;
	if(!reader.read8(versionAndHeaderLength) || ((versionAndHeaderLength & 0xf0) != 0x40))
		goto formatError;

	if(!reader.skipAhead(1))
		goto formatError;

	if(!reader.read16net(ipLength))
		goto formatError;

	if(!reader.read16net(id))
		goto formatError;

	if(!reader.read16net(fragmentOffsetAndFlags))
		goto formatError;

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
	if(!reader.skipAhead(1))
		goto formatError;

	if(!reader.read8(protocol))
		goto formatError;

	if(!reader.skipAhead(2))
		goto formatError;

	if(!reader.read32net(srcIp.addr) || !reader.read32net(dstIp.addr))
		goto formatError;

	if(!reader.skipAhead(static_cast<uint16_t>(reader.remainingLength)))
		goto formatError;

	NET_ASSERT(reader.isDone());

	if(!reader.valid)
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

	switch(protocol) {
	case IpProtocolNumbers::icmp:
		handler = checkIcmpPacket(reader);
		break;
	case IpProtocolNumbers::tcp:
	case IpProtocolNumbers::udp: {
			auto payloadLength = static_cast<uint16_t>(ipLength - ((versionAndHeaderLength & 0x0f) << 2));
			reader.InetChecksumDigester::reset();
			reader.remainingLength = payloadLength;
			reader.offsetOdd = false;
			reader.patch(0, correctEndian(static_cast<uint16_t>(srcIp.addr >> 16)));
			reader.patch(0, correctEndian(static_cast<uint16_t>(srcIp.addr & 0xffff)));
			reader.patch(0, correctEndian(static_cast<uint16_t>(dstIp.addr >> 16)));
			reader.patch(0, correctEndian(static_cast<uint16_t>(dstIp.addr & 0xffff)));
			auto chunk = reader.getChunk();
			reader.consume(chunk.start, chunk.length());

			if(protocol == 6) {
				handler = checkTcpPacket(reader, payloadLength);
			} else {
				handler = checkUdpPacket(reader, payloadLength);
			}

			break;
		}
	}

    if(handler) {
        NET_ASSERT(!reader.skipAhead(0xffff));

        if(reader.totalLength != ipLength)
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
    Packet packet;

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
        if(!packet.isValid() && data.packets.takePacketFromQueue(packet)) {
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
