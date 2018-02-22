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
	friend class TcpRxJob::PacketReaderBase;

	typedef TcpRx<Socket> Self;
	typedef typename InputChannel::DataWaiter Data;

    /**
     * The chain of data chucks that is being read.
     *
     * The reference to the currently accessed data needs to be copied,
     * in a synchronized context, to avoid concurrent access with the
     * handler of the reception of more incoming data.
     */
	DataChain localData;

    /**
     * Internal callback!
     *
     * It is called by IoRequest to decide whether it should block or not.
     */
    inline bool onBlocking()
    {
    	/*
    	 * Try to fetch a packet, this method is already contention protected context.
    	 */
		this->receivedData.sweepOutTo(localData);
		return localData.isEmpty();
    }

	inline bool takeNext() {
		if(this->current)
			state.pool.template freeSingle<Pool::Quota::Rx>(this->current);

		if(Block* next = this->localData.get()) {
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

        if(result == Result::Canceled) {
        	self->PacketStreamState::invalidate();
        	self->receivedData.template dropAll<Pool::Quota::Rx>();
        	self->localData.template dropAll<Pool::Quota::Rx>();
            return false;
        }

        return true;
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
