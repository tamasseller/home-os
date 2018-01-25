/*
 * TcpInputChannel.h
 *
 *  Created on: 2018.01.21.
 *      Author: tooma
 */

#ifndef TCPINPUTCHANNEL_H_
#define TCPINPUTCHANNEL_H_

#include "Network.h"

template<class S, class... Args>
class Network<S, Args...>::TcpCore::InputChannel:
	PacketInputChannel<ConnectionTag>,
	Os::template SynchronousIoChannelBase<InputChannel>
{
	typedef typename InputChannel::PacketInputChannel RxChannel;
	typedef typename Os::template SynchronousIoChannelBase<InputChannel> WindowChannel;
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
		friend RxChannel;
		uint16_t requestedWindowSize;

		inline SocketData* getSocketData();
	};

	class SocketData: public RxChannel::IoData, WindowWaiterData {
		friend RxChannel;
		friend class TcpListener;
		friend class TcpRetransmitJob;

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

		/// Network layer address of the peer.
		AddressIp4 peerAddress;

		/// TCP port number of the peer.
		uint16_t peerPort;

		/*
		 *	RFC793 Figure 4 (Send Sequence Space):
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
		uint16_t receiveWindow;

		State state;

		// TODO timers: retransmission, zero window probe
	};

private:
	/**
	 * IoChannel implementation used for the WindowWaiter side.
	 */
	inline bool addItem(typename Os::IoJob::Data* data){
		// TODO check state
		return true;
	}

	/**
	 * IoChannel implementation used for the WindowWaiter side.
	 */
	inline bool removeCanceled(typename Os::IoJob::Data* data) {
		// TODO check state
		return true;
	}

public:
	inline PacketInputChannel<ConnectionTag> *getInputChannel() {
		return static_cast<RxChannel*>(this);
	}

	inline typename Os::template SynchronousIoChannelBase<InputChannel>* getWindowWaiterChannel() {
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
		if(typename RxChannel::IoData* listener = this->findListener(tag)) {
			listener->packets.put(p);
			this->RxChannel::jobDone(listener);
			return true;
		}

		return false;
	}
};

template<class S, class... Args>
inline typename Network<S, Args...>::TcpCore::InputChannel::SocketData*
Network<S, Args...>::TcpCore::InputChannel::WindowWaiterData::getSocketData() {
	return static_cast<SocketData*>(this);
}

#endif /* TCPINPUTCHANNEL_H_ */
