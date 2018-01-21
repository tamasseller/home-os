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
class Network<S, Args...>::TcpSocket {
	friend typename TcpCore::Listener;
	friend typename TcpCore::RetransmitJob;
	friend pet::LinkedList<TcpSocket>;

	typename TcpCore::InputChannel::SocketData data;

	TcpSocket *next;
};

#endif /* TCPSOCKET_H_ */
