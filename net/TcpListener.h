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
    protected Os::template IoRequest<IpRxJob<TcpListener, TcpListenerChannel>, &IpRxJob<TcpListener, TcpListenerChannel>::onBlocking>
{
	friend class TcpListener::IpRxJob;

	AddressIp4 peerAddress;
    uint32_t initialReceivedSequenceNumber;
    uint16_t peerPort;

    const char* error;

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
        NET_ASSERT(this->read32net(initialReceivedSequenceNumber));

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
		state.tcpRstJob.handlePacket(this->packet);
		this->invalidateAllStates();
	}

	bool accept(TcpSocket& socket)
	{
		if(socket.data.state != TcpInputChannel::SocketData::State::Closed) {
			error = NetErrorStrings::alreadyConnected;
			return false;
		}

		state.increment(&DiagnosticCounters::Tcp::inputConnectionAccepted);


		uint32_t initialSendSequenceNumber = 0; // TODO time based generator.

		socket.data.state = TcpInputChannel::SocketData::State::SynReceived;

		// Receive sequence space parameters.
		socket.data.expectedSequenceNumber = initialReceivedSequenceNumber + 1;
		socket.data.receiveWindow = 1024; // TODO add magic here.

		// Send sequence space parameters.
		socket.data.lastReceivedAckNumber = initialSendSequenceNumber;
		socket.data.nextSequenceNumber = initialSendSequenceNumber + 1;
		socket.data.peerWindowSize = 0;

		// TODO generate SYN-ACK packet (with ACK = initialReceivedSequenceNumber, SEQ = initialSendSequenceNumber)
		// TODO send syn packet.
		// TODO add to the retranmit queue

		return true;
	}

	bool listen(uint16_t port)
	{
		this->data.accessTag() = DstPortTag(port);
		return this->launch(&TcpListener::IpRxJob::startReception, &state.tcpListenerChannel);
	}

	void close()
	{
		this->cancel();
		this->wait();
	}
};

#endif /* TCPLISTENER_H_ */
