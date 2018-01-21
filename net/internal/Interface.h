/*
 * Interface.h
 *
 *  Created on: 2018.01.21.
 *      Author: tooma
 */

#ifndef INTERFACE_H_
#define INTERFACE_H_

#include "Network.h"

template<class S, class... Args>
class Network<S, Args...>::Interface: public Os::IoChannel, Pool::IoData, Os::IoJob {
    template<class> friend class NetworkTestAccessor;
	friend class Network<S, Args...>;

public:
	struct AddressResolver: Os::IoChannel {
		virtual const AddressEthernet& getAddress() = 0;
		virtual bool resolveAddress(AddressIp4 ip, AddressEthernet& mac) = 0;
		virtual void fillHeader(PacketBuilder&, const AddressEthernet& dst, uint16_t etherType) = 0;
	};

    inline Interface(uint16_t headerSize, AddressResolver* resolver): resolver(resolver), headerSize(headerSize) {}

	inline void ageContent() {}

	inline AddressResolver *getResolver() {
        return resolver;
    }

	inline typename Os::IoChannel *getSender() {
        return static_cast<typename Os::IoChannel*>(this);
    }

    inline uint16_t getHeaderSize() {
        return headerSize;
    }

    inline void requestRxBuffers(uint16_t n)
    {
        typename Pool::Allocator ret = state.pool.template allocateDirect<Pool::Quota::Rx>(n);

        if(!ret.hasMore()) {
            this->nBuffersRequested.increment(n);
            this->launch(&Interface::acquireBuffers);
        } else {
            this->takeRxBuffers(ret);
        }
	}

    virtual void takeRxBuffers(typename Pool::Allocator allocator) = 0;

    inline void ipPacketReceived(Packet packet) {
    	Network<S, Args...>::ipPacketReceived(packet, this);
    };

private:
    using IoJob = typename Os::IoJob;
	using Result = typename IoJob::Result;
	using Launcher = typename IoJob::Launcher;
	using Callback = typename IoJob::Callback;

	AddressResolver* const resolver;
    typename Os::template Atomic<uint16_t> nBuffersRequested;
	uint16_t headerSize;

    static inline bool buffersAcquired(Launcher *launcher, IoJob* item, Result result)
    {
    	auto self = static_cast<Interface*>(item);
    	NET_ASSERT(result == Result::Done);

    	self->takeRxBuffers(static_cast<typename Pool::IoData*>(self)->allocator);

    	if(uint16_t n = self->nBuffersRequested.reset()) {
        	return state.pool.template allocateDirectOrDeferred<Pool::Quota::Rx>(
        			launcher,
        			&Interface::buffersAcquired,
        			static_cast<typename Pool::IoData*>(self),
        			n);
    	} else
    		return false;
    }

    static inline bool acquireBuffers(Launcher *launcher, IoJob* item)
    {
		auto self = static_cast<Interface*>(item);

		self->request.size = self->nBuffersRequested.reset();
        self->request.quota = Pool::Quota::Rx;

   		launcher->launch(&state.pool, &Interface::buffersAcquired, static_cast<typename Pool::IoData*>(self));
   		return true;
    }
};

#endif /* INTERFACE_H_ */
