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
struct Network<S, Args...>::TcpCore::TcpRxJob:
	public PacketReaderBase<TcpRxJob<Socket>>,
	PacketStreamState,
	Os::IoJob,
	InputChannel::DataWaiter
{
	typedef TcpRx<Socket> Self;
	typedef typename InputChannel::DataWaiter Data;

	friend class TcpRxJob::PacketReaderBase;

	inline bool takeNext() {
		if(this->current)
			state.pool.template freeSingle<Pool::Quota::Rx>(this->current);

		if(Block* next = this->receivedData.get()) { // XXX this is racy as fuck TODO fix race with disabling IpCore
			this->updateDataPointers(next);
			return true;
		} else {
			this->updateDataPointers(nullptr);
			return false;
		}
	}

    static bool received(Launcher *launcher, IoJob* item, Result result)
    {
        auto self = static_cast<Self*>(item);

        if(result == Result::Done) {
            // self->fetchPacket();
            return true;
        } else {
        	self->PacketStreamState::invalidate();
        	static_cast<typename InputChannel::DataWaiter*>(self)->receivedData.template dropAll<Pool::Quota::Rx>();
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
