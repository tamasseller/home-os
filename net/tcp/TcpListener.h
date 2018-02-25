/*
 * TcpListener.h
 *
 *  Created on: 2018.01.18.
 *      Author: tooma
 */

#ifndef TCPLISTENER_H_
#define TCPLISTENER_H_

template<class S, class... Args>
class Network<S, Args...>::TcpListener:
    protected Os::template IoRequest<IpRxJob<TcpListener, typename TcpCore::ListenerChannel>>
{
	friend class TcpListener::IpRxJob;

    const char* error;

    AddressIp4 peerAddress;
    Packet packet;
    uint32_t initialReceivedSequenceNumber;
    uint16_t peerPort;
    uint16_t peerMss;

    inline void reset() {
        peerAddress = AddressIp4::allZero;
    	peerPort = 0;
    	peerMss = 536;
    }

    inline void dispose(Packet packet) {
    	if(this->packet.isValid()) {
    		NET_ASSERT(packet == this->packet);
    		packet.template dispose<Pool::Quota::Rx>();
    	}
    }

    inline void preprocess(Packet packet)
    {
        using namespace TcpPacket;

    	auto ipAccessor = Fixup::template extractAndSkip<PacketStream, IpPacket::SourceAddress>(*this);
    	auto &stream = *static_cast<PacketStream*>(this);

        StructuredAccessor<SourcePort, SequenceNumber, Flags, End> tcpAccessor;
        NET_ASSERT(tcpAccessor.extract(stream));

        this->peerPort = tcpAccessor.get<SourcePort>();
        this->initialReceivedSequenceNumber = tcpAccessor.get<SequenceNumber>();
        this->peerAddress = ipAccessor.template get<IpPacket::SourceAddress>();
        this->packet = packet;
        this->peerMss = 536;

        auto optionBytes = tcpAccessor.get<Flags>().getDataOffset() - 20;

        while(optionBytes) {
        	uint8_t kind;
			NET_ASSERT(stream.read8(kind));

        	switch(kind) {
        	case 0:
        	case 1:
        		optionBytes -= 1;
        		break;
        	default:
        		uint8_t length;
        		NET_ASSERT(stream.read8(length));

        		if(length <= optionBytes) {
        			optionBytes -= length;
        		} else {
        			stream.skipAhead(static_cast<uint16_t>(optionBytes - 2));
        			optionBytes = 0;
        			break;
        		}

        		if(kind == 2 && length == 4) {
        			NET_ASSERT(stream.read16net(this->peerMss));
        		} else {
        			stream.skipAhead(static_cast<uint16_t>(length - 2));
        		}
        	}
        }

        state.increment(&DiagnosticCounters::Tcp::inputProcessed);
    }

	bool check() {
		if(this->isOccupied())
			this->wait();

		return error == nullptr;
	}

public:
    inline TcpListener() = default;
    inline TcpListener(const Initializer&) { init(); }

    using TcpListener::IoRequest::wait;

    AddressIp4 getPeerAddress() const {
        return peerAddress;
    }

    uint16_t getPeerPort() const {
        return peerPort;
    }

    const char* getError() {
    	return error;
    }

	void init()
	{
	    TcpListener::IpRxJob::init();
	    TcpListener::IoRequest::init();
	    error = nullptr;
	}

	void deny()
	{
		state.increment(&DiagnosticCounters::Tcp::inputConnectionDenied);

		Packet packet = this->packet;
    	this->packet.init(nullptr);
		this->invalidateAllStates();

		state.tcpCore.rstJob.handlePacket(packet);
	}

	bool accept(TcpSocket& socket)
	{
		if(socket.state != TcpSocket::State::Closed) {
			error = NetErrorStrings::alreadyConnected;
			return false;
		}

		if(!check())
			return false;

		state.increment(&DiagnosticCounters::Tcp::inputConnectionAccepted);

		uint32_t initialSendSequenceNumber = 0; // TODO time based generator.

		socket.accessTag() = ConnectionTag(peerAddress, peerPort, this->data.accessTag().getPortNumber());

		socket.state = TcpSocket::State::SynReceived;

		// Receive sequence space parameters.
		socket.expectedSequenceNumber = initialReceivedSequenceNumber + 1;
		socket.lastAdvertisedReceiveWindow = socket.getReceiveWindow();

		// Send sequence space parameters.
		socket.lastReceivedAckNumber = initialSendSequenceNumber;
		socket.nextSequenceNumber = initialSendSequenceNumber;
		socket.peerWindowSize = 0;

		socket.maximumTxSegmentSize = peerMss;

		if(!socket.getRx().launch(&TcpSocket::TcpRx::startReception))
			return false;

		socket.sendSynAck();

		this->invalidateAllStates();

		return true;
	}

	bool listen(uint16_t port)
	{
		this->data.accessTag() = DstPortTag(port);
		return this->launch(&TcpListener::IpRxJob::startReception, &state.tcpCore.listenerChannel);
	}

	void close()
	{
		this->cancel();
		this->wait();
	}
};

#endif /* TCPLISTENER_H_ */
