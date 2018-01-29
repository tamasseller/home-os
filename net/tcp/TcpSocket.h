/*
 * TcpSocket.h
 *
 *  Created on: 2018.01.21.
 *      Author: tooma
 */

#ifndef TCPSOCKET_H_
#define TCPSOCKET_H_

#include "Network.h"

template<class S, class... Args>
class Network<S, Args...>::TcpSocket:
	TcpCore::template TcpTx<TcpSocket>,
	TcpCore::template TcpRx<TcpSocket>
{
	friend typename TcpCore::template TcpTx<TcpSocket>;
	friend typename TcpCore::template TcpRx<TcpSocket>;
	friend typename TcpCore::InputChannel::WindowWaiter;
	friend typename TcpCore::InputChannel::DataWaiter;
	friend typename TcpCore::RetransmitJob;
	friend pet::LinkedList<TcpSocket>;
	friend TcpListener;

    TcpSocket *next;

    /**
     *  States of the TCP state machine (as defined in RFC793 3.2)
     *
     *  The LISTEN state is not needed as passive opens are
     *  implemented using separate mechanism from the actual communication sockets.
     *
     *  The TIME WAIT state is not implemented (due to resource allocation considerations).
     */
    enum State: uint8_t {
        /**
         * RFC793 3.2: represents no connection state at all.
         */
        Closed,

        /**
         *  RFC793 3.2: represents waiting for a matching connection request after
         *  having sent a connection request.
         */
        SynSent,

        /**
         *  RFC793 3.2: represents waiting for a confirming connection request acknowledgment
         *  after having both received and sent a connection request.
         */
        SynReceived,

        /**
         * RFC793 3.2: represents an open connection, data received can be delivered to
         * the user.  The normal state for the data transfer phase of the connection.
         */
        Established,

        /**
         * RFC793 3.2: represents waiting for a connection termination request from the remote
         * TCP, or an acknowledgment of the connection termination request previously sent.
         */
        FinWait1,

        /**
         * RFC793 3.2: represents waiting for a connection termination request from the remote TCP.
         */
        FinWait2,

        /**
         * RFC793 3.2: represents waiting for a connection termination request from the local user.
         */
        CloseWait,

        /**
         * RFC793 3.2: represents waiting for a connection termination request acknowledgment from
         * the remote TCP.
         */
        Closing,

        /**
         * RFC793 3.2: represents waiting for an acknowledgment of the connection termination
         * request previously sent to the remote TCP (which includes an acknowledgment of its
         * connection termination request).
         */
        LastAck
    };

    /// Acknowledged packets (payload only).
    PacketChain receivedData;

    /// Unacknowledged data for which a retransmission may be needed.
    PacketChain unackedPackets;

    /*
     *  RFC793 Figure 4 (Send Sequence Space):
     *
     *            1         2          3          4
     *       ----------|----------|----------|----------
     *              SND.UNA    SND.NXT    SND.UNA
     *                                   +SND.WND
     *
     *  1 - old sequence numbers which have been acknowledged
     *  2 - sequence numbers of unacknowledged data
     *  3 - sequence numbers allowed for new data transmission
     *  4 - future sequence numbers which are not yet allowed
     */

    /// RFC793 SND.WND - The last accepted window update.
    uint16_t peerWindowSize;

    /// RFC793 SND.UNA - The last accepted ACK number.
    uint32_t lastReceivedAckNumber;

    /// RFC793 SND.NXT - The next sequence number to be used for TX.
    uint32_t nextSequenceNumber;

    /// RFC793 SND.WL1 - The sequence number of last accepted window update segment.
    uint32_t lastWindowUpdateSeqNum;

    /// RFC793 SND.WL2 - The acknowledgment number of last accepted window update segment.
    uint32_t lastWindowUpdateAckNum;

    /*
     *  RFC793 Figure 5 (Receive Sequence Space):
     *
     *                  1          2          3
     *             ----------|----------|----------
     *                    RCV.NXT    RCV.NXT
     *                              +RCV.WND
     *  1 - old sequence numbers which have been acknowledged
     *  2 - sequence numbers allowed for new reception
     *  3 - future sequence numbers which are not yet allowed
     */

    /// RFC793 RCV.NXT - The sequence number expected for the next acceptable packet.
    uint32_t expectedSequenceNumber;

    /// RFC793 RCV.WND - receive window
    uint16_t lastAdvertisedReceiveWindow;

    State state = State::Closed;

    // TODO timers: retransmission, zero window probe

    inline uint16_t getReceiveWindow() {
    	return 1024; // TODO add magic here.
    }

    inline TcpPacket::FullHeaderAccessor getTxHeader()
    {
    	using namespace TcpPacket;
    	FullHeaderAccessor accessor;

    	auto window = getReceiveWindow();

        accessor.get<SourcePort>()            = this->accessTag().getLocalPort();
        accessor.get<DestinationPort>()       = this->accessTag().getPeerPort();
        accessor.get<SequenceNumber>()        = nextSequenceNumber;
        accessor.get<AcknowledgementNumber>() = expectedSequenceNumber;

        accessor.get<WindowSize>()            = window;
    	lastAdvertisedReceiveWindow           = window;

        accessor.get<Checksum>()              = 0;
        accessor.get<UrgentPointer>()         = 0;
        accessor.get<Flags>().clear();

        return accessor;
    }

public:
    auto &getTx() {
    	return *static_cast<typename TcpCore::template TcpTx<TcpSocket>*>(this);
    }

    auto &getRx() {
    	return *static_cast<typename TcpCore::template TcpRx<TcpSocket>*>(this);
    }

    inline void init() {
    	getTx().init();
    	// getRx().init();
    }

    inline void abandon	() {
    	getTx().abandon();
    	// getRx().abandon();
    }
};

#endif /* TCPSOCKET_H_ */
