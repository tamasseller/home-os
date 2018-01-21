/*
 * RawTransmitter.h
 *
 *  Created on: 2018.01.21.
 *      Author: tooma
 */

#ifndef RawTransmitter_H_
#define RawTransmitter_H_

#include "Network.h"

template<class S, class... Args>
class Network<S, Args...>::RawTransmitter: public IpTransmitterBase<RawTransmitter> {
	friend class RawTransmitter::IpTransmitterBase::IpTxJob;
	static inline bool onPreparationDone(Launcher *launcher, IoJob* item) { return false; }
public:

	inline bool prepare(AddressIp4 dst, size_t inLineSize, size_t indirectCount = 0)
	{
    	this->reset();
		bool later = this->launch(&RawTransmitter::IpTransmitterBase::IpTxJob::startPreparation, dst, inLineSize, indirectCount, arpRequestRetry);
		return later || this->getError() == nullptr;
	}

    inline bool prepareTimeout(size_t timeout, AddressIp4 dst, size_t inLineSize, size_t indirectCount = 0)
    {
    	this->reset();
        bool later = this->launchTimeout(&RawTransmitter::IpTransmitterBase::IpTxJob::startPreparation, timeout, dst, inLineSize, indirectCount, arpRequestRetry);
        return later || this->getError() == nullptr;
    }

	inline bool send(uint8_t protocol, uint8_t ttl = 64)
	{
		if(!this->check())
			return false;

		bool later = this->launch(&RawTransmitter::IpTransmitterBase::IpTxJob::startTransmission, protocol, ttl);
        return later || this->getError() == nullptr;
	}

    inline bool sendTimeout(size_t timeout, uint8_t protocol, uint8_t ttl = 64)
    {
        if(!this->check())
            return false;

        bool later = this->launchTimeout(&RawTransmitter::IpTransmitterBase::IpTxJob::startTransmission, timeout, protocol, ttl);
        return later || this->getError() == nullptr;
    }
};

#endif /* RawTransmitter_H_ */
