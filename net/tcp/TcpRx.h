/*
 * TcpRx.h
 *
 *  Created on: 2018.01.29.
 *      Author: tooma
 */

#ifndef TCPRX_H_
#define TCPRX_H_

#include "Network.h"
#include "TcpCore.h"

template<class S, class... Args>
template<class Socket>
struct Network<S, Args...>::TcpCore::TcpRx: Os::IoJob, TcpCore::InputChannel::DataWaiter {

};

#endif /* TCPRX_H_ */
