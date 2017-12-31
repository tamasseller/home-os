/*
 * IpReplyJob.h
 *
 *  Created on: 2017.12.30.
 *      Author: tooma
 */

#ifndef IPREPLYJOB_H_
#define IPREPLYJOB_H_

#include "Network.h"

template<class S, class... Args>
template<class Child>
class Network<S, Args...>::IpReplyJob: IpTxJob<IpReplyJob<Child>>, public RxPacketHandler
{
	friend class IpReplyJob::IpTxJob;

protected:
	struct InitialReplyInfo {
		AddressIp4 dst;
		uint16_t length;
	};

	struct FinalReplyInfo {
		uint8_t protocol, ttl;
	};

private:
    PacketQueue requestQueue;
    Packet request;

    inline bool restartIfNeeded(Launcher *launcher, IoJob* item)
    {
        if(!this->requestQueue.isEmpty()) {
            startReplySequence(launcher, item);
            return true;
        }

        return false;
    }

    inline bool onPrepFailed(Launcher *launcher, IoJob* item) {
        this->request.template dispose<Pool::Quota::Rx>();
        return restartIfNeeded(launcher, item);
    }

    inline bool onSent(Launcher *launcher, IoJob* item, Result result) {
    	this->disposePacket();
    	return restartIfNeeded(launcher, item);
    }

	inline bool onPreparationDone(Launcher *launcher, IoJob* item)
	{
    	FinalReplyInfo info = static_cast<Child*>(this)->generateReply(request, this->accessPacket());

		request.template dispose<Pool::Quota::Rx>();
		return IpReplyJob::IpTxJob::startTransmission(launcher, item, info.protocol, info.ttl);
	}

    inline bool onNoRoute(Launcher *launcher, IoJob* item) {
        return onPrepFailed(launcher, item);
    }

    inline bool onArpTimeout(Launcher *launcher, IoJob* item) {
        return onPrepFailed(launcher, item);
    }

    /*
     * These methods should never be called. (LCOV_EXCL_START)
     */

    inline bool onPreparationAborted(Launcher *launcher, IoJob* item, Result result) {
        Os::assert(false, NetErrorStrings::unknown);
        return false;
    }

    /*
     * These methods should never be called. (LCOV_EXCL_STOP)
     */
    static bool startReplySequence(Launcher *launcher, IoJob* item) {
    	auto self = static_cast<IpReplyJob*>(item);

    	self->requestQueue.takePacketFromQueue(self->request);

    	InitialReplyInfo info = static_cast<Child*>(self)->getReplyInfo(self->request);

		return IpReplyJob::IpTxJob::startPreparation(launcher, item, info.dst, info.length, 0, 0);
    }


public:
    virtual void handlePacket(Packet packet) {
    	this->requestQueue.putPacketChain(packet);
		this->launch(&IpReplyJob::startReplySequence);
    }
};

#endif /* IPREPLYJOB_H_ */
