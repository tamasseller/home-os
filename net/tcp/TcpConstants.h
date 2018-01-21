/*
 * TcpConstants.h
 *
 *  Created on: 2018.01.21.
 *      Author: tooma
 */

#ifndef TCPCONSTANTS_H_
#define TCPCONSTANTS_H_

namespace TcpConstants {
	static constexpr uint16_t finMask = 0x0001;
	static constexpr uint16_t synMask = 0x0002;
	static constexpr uint16_t rstMask = 0x0004;
	static constexpr uint16_t ackMask = 0x0010;

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
}

#endif /* TCPCONSTANTS_H_ */
