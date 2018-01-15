/*
 * Tcp.h
 *
 *  Created on: 2018.01.12.
 *      Author: tooma
 */

#ifndef TCP_H_
#define TCP_H_

template<class S, class... Args>
template<class Reader>
inline typename Network<S, Args...>::RxPacketHandler* Network<S, Args...>::checkTcpPacket(Reader& reader, uint16_t length)
{
	static constexpr uint16_t ackMask = 0x01;
	static constexpr uint16_t synMask = 0x01;

	uint16_t offsetAndFlags, payloadLength;
	uint8_t offset;

	state.increment(&DiagnosticCounters::Tcp::inputReceived);

	if(length < 20)
		goto formatError;

	if(!reader.skipAhead(12) || !reader.read16net(offsetAndFlags))
		goto formatError;

	offset = static_cast<uint8_t>((offsetAndFlags >> 6) & ~0x3);

	if(length < offset)
		goto formatError;

	NET_ASSERT(!reader.skipAhead(0xffff));
	reader.patch(0, correctEndian(static_cast<uint16_t>(length)));
	reader.patch(0, correctEndian(static_cast<uint16_t>(IpProtocolNumbers::tcp)));

	if(reader.result())
		goto formatError;

	return &state.tcpPacketProcessor;

formatError:

	state.increment(&DiagnosticCounters::Tcp::inputFormatError);
	return nullptr;
}

template<class S, class... Args>
struct Network<S, Args...>::TcpPacketProcessor: PacketProcessor, RxPacketHandler {
    inline TcpPacketProcessor(): PacketProcessor(&Network<S, Args...>::processTcpPacket) {}

private:
    virtual void handlePacket(Packet packet) {
        this->process(packet);
    }
};

template<class S, class... Args>
inline void Network<S, Args...>::processTcpPacket(typename Os::Event*, uintptr_t arg)
{
	static constexpr uint16_t synMask = 0x0002;
	static constexpr uint16_t ackMask = 0x0010;

    Packet chain;
    chain.init(reinterpret_cast<Block*>(arg));

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
        	bool isInitial = (offsetAndFlags & synMask) && !(offsetAndFlags & ackMask);
        	if(!isInitial || !state.tcpListenerChannel.takePacket(start, DstPortTag(localPort))) {
            	state.increment(&DiagnosticCounters::Tcp::inputNoPort);
            	state.tcpRstJob.handlePacket(start);
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

	inline typename Base::FinalReplyInfo generateReply(Packet& request, TxPacketBuilder& reply)
	{
    	uint32_t seqNum;
        uint16_t peerPort, localPort;
        uint8_t ihl;

		PacketStream reader;
		reader.init(request);

        NET_ASSERT(reader.read8(ihl));
        NET_ASSERT((ihl & 0xf0) == 0x40);
        NET_ASSERT(reader.skipAhead(static_cast<uint16_t>(((ihl & 0x0f) << 2) - 1)));

        NET_ASSERT(reader.read16net(peerPort));
        NET_ASSERT(reader.read16net(localPort));
        NET_ASSERT(reader.read32net(seqNum));

	    static constexpr const char boilerplate[] = {
	    		/* off+flag / wnd siz |  checksum | urgent bs */
	    		0x05, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	    };

		NET_ASSERT(reply.write16net(localPort));
		NET_ASSERT(reply.write16net(peerPort));
		NET_ASSERT(reply.write32net(0x00));
		NET_ASSERT(reply.write32net(seqNum + 1));
	    reply.copyIn(boilerplate, sizeof(boilerplate));

		return typename Base::FinalReplyInfo{IpProtocolNumbers::tcp, 0xff};
	}
};

template<class S, class... Args>
class Network<S, Args...>::TcpAckJob: public IpTxJob<TcpAckJob> {
	// TODO reply job style, but with sockets instead of packets as its input.
	// TODO should send an empty acknowledge packet with the current data.
};

template<class S, class... Args>
class Network<S, Args...>::TcpInputChannel:
	PacketInputChannel<ConnectionTag>,
	Os::template SynchronousIoChannelBase<TcpInputChannel>
{
	typedef typename TcpInputChannel::PacketInputChannel InputChannel;
	typedef typename Os::template SynchronousIoChannelBase<TcpInputChannel> WindowChannel;
	friend WindowChannel;

public:
	class WindowWaiterData;

	class SocketData: public InputChannel::IoData {
		friend class TcpInputChannel;
		class WindowWaiterData* windowWaiter;

		uint32_t lastAckNumber;
		uint32_t expectedSequenceNumber;
		uint16_t lastPeerWindowSize;

		enum State: uint8_t {
			SynSent, SynReceived, Established
		} state;
	};

	class WindowWaiterData: public Os::IoJob::Data {
		friend class TcpInputChannel;
		WindowWaiterData* next;

	public:
		SocketData* socket;
		uint16_t requestedWindowSize;
	};

private:
	inline bool addItem(typename Os::IoJob::Data* data){
		auto windowWaiterData = static_cast<WindowWaiterData*>(data);
		Os::assert(!windowWaiterData->socket->windowWaiter, NetErrorStrings::socketConcurrency);
		windowWaiterData->socket->windowWaiter = windowWaiterData;
		return true;
	}

	inline bool removeCanceled(typename Os::IoJob::Data* data) {
		auto windowWaiterData = static_cast<WindowWaiterData*>(data);
		Os::assert(windowWaiterData->socket->windowWaiter == windowWaiterData, NetErrorStrings::socketConcurrency);
		windowWaiterData->socket->windowWaiter = windowWaiterData;
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

   	// TODO normal, per-socket checking and ACK reply and state update (should call into
	// some other class that knows about the per-socket info).
	//
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

#endif /* TCP_H_ */
