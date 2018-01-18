/*
 * Tcp.h
 *
 *  Created on: 2018.01.12.
 *      Author: tooma
 */

#ifndef TCP_H_
#define TCP_H_

namespace TcpConstants {
	static constexpr uint16_t finMask = 0x0001;
	static constexpr uint16_t synMask = 0x0002;
	static constexpr uint16_t rstMask = 0x0004;
	static constexpr uint16_t ackMask = 0x0010;
}


template<class S, class... Args>
template<class Reader>
inline typename Network<S, Args...>::RxPacketHandler* Network<S, Args...>::checkTcpPacket(Reader& reader, uint16_t length)
{
	using namespace TcpConstants;
    static struct TcpPacketHandler: RxPacketHandler {
        virtual void handlePacket(Packet packet) {
            Network<S, Args...>::processTcpPacket(packet);
        }
    } tcpPacketHandler;

	uint16_t offsetAndFlags, payloadLength;
	uint8_t offset;

	state.increment(&DiagnosticCounters::Tcp::inputReceived);

	if(length < 20)
		goto formatError;

	if(!reader.skipAhead(12) || !reader.read16net(offsetAndFlags))
		goto formatError;

	offset = static_cast<uint8_t>((offsetAndFlags >> 10) & ~0x3);

	if(length < offset)
		goto formatError;

	NET_ASSERT(!reader.skipAhead(0xffff));
	reader.patch(0, correctEndian(static_cast<uint16_t>(length)));
	reader.patch(0, correctEndian(static_cast<uint16_t>(IpProtocolNumbers::tcp)));

	if(reader.result())
		goto formatError;

	return &tcpPacketHandler;

formatError:

	state.increment(&DiagnosticCounters::Tcp::inputFormatError);
	return nullptr;
}

template<class S, class... Args>
inline void Network<S, Args...>::processTcpPacket(Packet chain)
{
	using namespace TcpConstants;
    PacketStream reader;
    reader.init(chain);

    while(true) {
        Packet start = reader.asPacket();

        uint8_t ihl;
        uint16_t peerPort, localPort;
        AddressIp4 peerAddress;
        NET_ASSERT(reader.read8(ihl));
        NET_ASSERT((ihl & 0xf0) == 0x40);

        NET_ASSERT(reader.skipAhead(11));
        NET_ASSERT(reader.read32net(peerAddress.addr));

        NET_ASSERT(reader.skipAhead(static_cast<uint16_t>(((ihl - 4) & 0x0f) << 2)));

        NET_ASSERT(reader.read16net(peerPort));
        NET_ASSERT(reader.read16net(localPort));

        NET_ASSERT(reader.skipAhead(8));

    	uint16_t offsetAndFlags;
        NET_ASSERT(reader.read16net(offsetAndFlags));

        bool hasMore = reader.cutCurrentAndMoveToNext();

        if(!state.tcpInputChannel.takePacket(start, ConnectionTag(peerAddress, peerPort, localPort))) {
        	if((offsetAndFlags & rstMask)) {
        		start.template dispose<Pool::Quota::Rx>();
        	} else {
				bool isInitial = (offsetAndFlags & synMask) && !(offsetAndFlags & ackMask);
				if(!isInitial || !state.tcpListenerChannel.takePacket(start, DstPortTag(localPort))) {
					state.increment(&DiagnosticCounters::Tcp::inputNoPort);
					state.tcpRstJob.handlePacket(start);
				}
        	}
        }

        if(!hasMore)
            break;
    }
}

template<class S, class... Args>
inline void Network<S, Args...>::tcpTxPostProcess(PacketStream& stream, InetChecksumDigester& checksum, size_t payload)
{
	checksum.patch(0, correctEndian(static_cast<uint16_t>(IpProtocolNumbers::tcp)));
	checksum.patch(0, correctEndian(static_cast<uint16_t>(payload)));

	NET_ASSERT(stream.skipAhead(16));
	NET_ASSERT(stream.write16raw(checksum.result()));

	state.increment(&DiagnosticCounters::Tcp::outputQueued);
}


template<class S, class... Args>
class Network<S, Args...>::TcpRstJob: public IpReplyJob<TcpRstJob, InetChecksumDigester>
{
	typedef typename TcpRstJob::IpReplyJob Base;
	friend typename Base::IpTxJob;
	friend Base;

	inline void replySent(Packet& request) {
		state.increment(&DiagnosticCounters::Tcp::outputSent);
	}

	inline void postProcess(PacketStream& stream, InetChecksumDigester& checksum, size_t payload) {
		tcpTxPostProcess(stream, checksum, payload);
	}

	inline typename Base::InitialReplyInfo getReplyInfo(Packet& request)
	{
        AddressIp4 peerAddress;
        uint8_t ihl;

		PacketStream reader;
		reader.init(request);

        NET_ASSERT(reader.read8(ihl));
        NET_ASSERT((ihl & 0xf0) == 0x40);

        NET_ASSERT(reader.skipAhead(11));
        NET_ASSERT(reader.read32net(peerAddress.addr));

		state.increment(&DiagnosticCounters::Tcp::rstPackets);

		return typename Base::InitialReplyInfo{peerAddress, static_cast<uint16_t>(20)};
	}

	inline typename Base::FinalReplyInfo generateReply(Packet& request, PacketBuilder& reply)
	{
		using namespace TcpConstants;
    	uint32_t seqNum, ackNum;
        uint16_t peerPort, localPort, payloadLength, flags;
        uint8_t ihl;

		PacketStream reader;
		reader.init(request);

        NET_ASSERT(reader.read8(ihl));
        NET_ASSERT(reader.skipAhead(1));

        NET_ASSERT(reader.read16net(payloadLength));
        payloadLength = static_cast<uint16_t>(payloadLength - ((ihl & 0x0f) << 2));

        NET_ASSERT((ihl & 0xf0) == 0x40); // IPv4 only for now.
        NET_ASSERT(reader.skipAhead(static_cast<uint16_t>((((ihl - 1) & 0x0f) << 2))));

        NET_ASSERT(reader.read16net(peerPort));
        NET_ASSERT(reader.read16net(localPort));
        NET_ASSERT(reader.read32net(seqNum));
        NET_ASSERT(reader.read32net(ackNum));
        NET_ASSERT(reader.read16net(flags));
        payloadLength = static_cast<uint16_t>(payloadLength - ((flags >> 10) & ~0x3));

        if(flags & synMask) payloadLength++;
        if(flags & finMask) payloadLength++;

		NET_ASSERT(reply.write16net(localPort));
		NET_ASSERT(reply.write16net(peerPort));

		NET_ASSERT(reply.write32net((flags & ackMask) ? ackNum : 0));
		NET_ASSERT(reply.write32net(seqNum + payloadLength));

		NET_ASSERT(reply.write16net(0x5014));	// Flags
		NET_ASSERT(reply.write16net(0));		// Window size
		NET_ASSERT(reply.write32net(0));		// Checksum and urgent pointer.

		return typename Base::FinalReplyInfo{IpProtocolNumbers::tcp, 0xff};
	}
};

template<class S, class... Args>
class Network<S, Args...>::TcpAckJob: public IpTxJob<TcpAckJob> {
	// TODO reply job style, but with sockets instead of packets as its input.

	/* TODO:
	 *		1. send an empty acknowledge packet with the current ack number
	 *		2. put packet into the acked list
	 */

};

template<class S, class... Args>
class Network<S, Args...>::TcpInputChannel:
	PacketInputChannel<ConnectionTag>,
	Os::template SynchronousIoChannelBase<TcpInputChannel>
{
	typedef typename TcpInputChannel::PacketInputChannel InputChannel;
	typedef typename Os::template SynchronousIoChannelBase<TcpInputChannel> WindowChannel;
	friend WindowChannel;

	uint32_t tcpTime;

	struct TcpTimer {
		class TcpTimer* next;
		uint32_t deadline;
	};

	struct RetransmissionTimer: TcpTimer{};

public:
	class SocketData;

	class WindowWaiterData: public Os::IoJob::Data {
		friend class TcpInputChannel;
		uint16_t requestedWindowSize;

		inline SocketData* getSocketData();
	};

	class SocketData: public InputChannel::IoData, WindowWaiterData {
		friend class TcpInputChannel;
		friend class TcpListener;

		AddressIp4 peerAddress;
		uint16_t peerPort;

		uint16_t rxWindowSize;			// Current receiver window size (RCV.WND in RFC793).
		uint16_t rxLastPeerWindowSize;	// Last received window size (SND.WND in RFC793).
		uint32_t rxNextSequenceNumber;  // The sequence number expected for the next packet (RCV.UNA in RFC793).
		uint32_t rxLastAckNumber;		// The last received acknowledgement number (SND.UNA in RFC793).
		uint32_t txNextSequenceNumber;	// The sequence number to be used for the next segment (SND.NXT in RFC793).

		PacketChain rxPackets;			// Received packets (payload only).
		PacketChain txPackets;			// Unacknowledged segments.

		// TODO timers: retransmission, zero window probe

		enum State: uint8_t {
			SynSent, SynReceived, Established
		} state;
	};

private:
	inline bool addItem(typename Os::IoJob::Data* data){
		// TODO check state
		return true;
	}

	inline bool removeCanceled(typename Os::IoJob::Data* data) {
		// TODO check state
		return true;
	}

public:
	inline PacketInputChannel<ConnectionTag> *getInputChannel() {
		return static_cast<InputChannel*>(this);
	}

	inline typename Os::template SynchronousIoChannelBase<TcpInputChannel>* getWindowWaiterChannel() {
		return static_cast<WindowChannel*>(this);
	}

	void init() {
		getInputChannel()->init();
	}

	// If sequence number is correct:
	//   - update lastAcked, and window, drop from unacked as needed, reset retransmission timer.
	//   - if data length is not zero:
	//   	- check if data fits the receive window (if not drop packet silently),
	//   	- update seqnum and the window size with the length,
	//   	- queue the socket for ACK packet transmission,
	//   	- queue the data for reading
	//   	- notify the reader.
	//
	// If the sequence number is wrong:
	//   - increment fast retransmit dupack counter
	//   - queue the socket for ACK packet transmission (with the old sequence number),
	//   - drop packet.
	//
	inline bool takePacket(Packet p, ConnectionTag tag)
	{
		if(typename InputChannel::IoData* listener = this->findListener(tag)) {
			listener->packets.putPacketChain(p);
			this->InputChannel::jobDone(listener);
			return true;
		}

		return false;
	}
};

template<class S, class... Args>
inline typename Network<S, Args...>::TcpInputChannel::SocketData*
Network<S, Args...>::TcpInputChannel::WindowWaiterData::getSocketData() {
	return static_cast<SocketData*>(this);
}


template<class S, class... Args>
class Network<S, Args...>::TcpListener:
    protected Os::template IoRequest<IpRxJob<TcpListener, TcpListenerChannel>, &IpRxJob<TcpListener, TcpListenerChannel>::onBlocking>
{
	friend class TcpListener::IpRxJob;

	AddressIp4 peerAddress;
    uint32_t initialSequenceNumber;
    uint16_t peerPort;

    inline void reset() {
        peerAddress = AddressIp4::allZero;
    	peerPort = 0;
    }

    inline void preprocess() {
        uint8_t ihl;
        NET_ASSERT(this->read8(ihl));
        NET_ASSERT((ihl & 0xf0) == 0x40); // IPv4 only for now.

        NET_ASSERT(this->skipAhead(11));
        NET_ASSERT(this->read32net(this->peerAddress.addr));

        NET_ASSERT(this->skipAhead(static_cast<uint16_t>(((ihl - 4) & 0x0f) << 2)));

        NET_ASSERT(this->read16net(peerPort));
        NET_ASSERT(this->skipAhead(2));
        NET_ASSERT(this->read32net(initialSequenceNumber));

        state.increment(&DiagnosticCounters::Tcp::inputProcessed);
    }

public:
    using TcpListener::IoRequest::wait;

    AddressIp4 getPeerAddress() const {
        return peerAddress;
    }

    uint16_t getPeerPort() const {
        return peerPort;
    }

	void init() {
	    TcpListener::IpRxJob::init();
	    TcpListener::IoRequest::init();
	}

	void deny() {
		state.increment(&DiagnosticCounters::Tcp::inputConnectionDenied);
		state.tcpRstJob.handlePacket(this->packet);
		this->invalidateAllStates();
	}

	bool accept(TcpSocket& socket) {
		state.increment(&DiagnosticCounters::Tcp::inputConnectionAccepted);
		// TODO check state;
		socket.data.state = TcpInputChannel::SocketData::State::SynReceived;
		socket.data.rxLastWindowSize = 1;
		socket.data.txPeerWindowLeft = 1;
		socket.data.rxNextSequenceNumber = initialSequenceNumber + 1;
		socket.data.txLastSequenceNumber = 0; // TODO generate randomly

		// TODO add syn packet as unacked
		// TODO queue for sending ack

		return true;
	}

	bool listen(uint16_t port) {
		this->data.accessTag() = DstPortTag(port);
		return this->launch(&TcpListener::IpRxJob::startReception, &state.tcpListenerChannel);
	}

	void close() {
		this->cancel();
		this->wait();
	}
};

template<class S, class... Args>
class Network<S, Args...>::TcpSocket {
	friend TcpListener;
	typename TcpInputChannel::SocketData data;
};

#endif /* TCP_H_ */
