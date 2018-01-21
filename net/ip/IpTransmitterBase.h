/*
 * IpTransmitterBase.h
 *
 *  Created on: 2018.01.21.
 *      Author: tooma
 */

#ifndef IPTRANSMITTERBASE_H_
#define IPTRANSMITTERBASE_H_

#include "Network.h"

template<class S, class... Args>
template<class Child>
class Network<S, Args...>::IpTransmitterBase: protected Os::template IoRequest<IpTxJob<Child>> {
	friend class IpTransmitterBase::IpTxJob;

	const char* error;

protected:
	inline bool onSent(Launcher *launcher, IoJob* item, Result result) {
        if(result != Result::Done)
            error = (result == Result::TimedOut) ? NetErrorStrings::genericTimeout : NetErrorStrings::genericCancel;

        this->disposePacket();
        return false;
	}

    inline bool onNoRoute(Launcher *launcher, IoJob* item) {
        error = NetErrorStrings::noRoute;
        return false;
    }

    inline bool onArpTimeout(Launcher *launcher, IoJob* item) {
        error = NetErrorStrings::unresolved;
        return false;
    }

    inline bool onPreparationAborted(Launcher *launcher, IoJob* item, Result result) {
        error = (result == Result::TimedOut) ? NetErrorStrings::genericTimeout : NetErrorStrings::genericCancel;
        return false;
    }

	bool check() {
		if(this->isOccupied())
			this->wait();

		return error == nullptr;
	}

	void reset() {
		if(this->isOccupied())
			this->wait();

		error = nullptr;
	}

public:
	using IpTransmitterBase::IoRequest::init;
	using IpTransmitterBase::IoRequest::isOccupied;
	using IpTransmitterBase::IoRequest::wait;
	using IpTransmitterBase::IoRequest::cancel;

	inline uint16_t fill(const char* data, uint16_t length)
	{
		if(!this->check())
			return false;

		return this->accessPacket().copyIn(data, length);
	}

    inline bool addIndirect(const char* data, uint16_t length, typename Block::Destructor destructor = nullptr, void* userData = nullptr) {
		if(!this->check())
			return false;

        return this->accessPacket().addByReference(data, length, destructor, userData);
    }

	inline const char* getError() {
		return this->error;
	}
};

#endif /* IPTRANSMITTERBASE_H_ */
