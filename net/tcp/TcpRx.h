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
struct Network<S, Args...>::TcpCore::TcpRxJob: Os::IoJob, TcpCore::InputChannel::DataWaiter
{
	typedef TcpRx<Socket> Self;
	typedef typename InputChannel::DataWaiter Data;

    static bool received(Launcher *launcher, IoJob* item, Result result)
    {
        auto self = static_cast<Self*>(item);

        if(result == Result::Done) {
            // self->fetchPacket();
            return true;
        } else {
            Packet packet;

        	// self->invalidateAllStates();

            // self->data.packets.template dropAll<Pool::Quota::Rx>();
            return false;
        }
    }

    static bool startReception(Launcher *launcher, IoJob* item)
    {
        auto self = static_cast<Self*>(item);
        launcher->launch(state.tcpCore.inputChannel.getInputChannel(), &Self::received, static_cast<Data*>(self));
        return true;
    }
};

template<class S, class... Args>
template<class Socket>
struct Network<S, Args...>::TcpCore::TcpRx: Os::template IoRequest<TcpCore::TcpRxJob<Socket>> {
	inline void abandon() {
		this->cancel();
		this->wait();
	}
};

#endif /* TCPRX_H_ */
