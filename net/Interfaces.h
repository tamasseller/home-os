/*
 * Interfaces.h
 *
 *  Created on: 2017.11.21.
 *      Author: tooma
 */

#ifndef INTERFACES_H_
#define INTERFACES_H_

#include "Network.h"

#include "meta/ApplyToPack.h"

template<class S, class... Args>
class Network<S, Args...>::Interface: public Os::IoChannel, Pool::IoData, Os::IoJob {
    template<class> friend class NetworkTestAccessor;
	friend class Network<S, Args...>;

public:
	struct AddressResolver: Os::IoChannel {
		virtual const AddressEthernet& getAddress() = 0;
		virtual bool resolveAddress(AddressIp4 ip, AddressEthernet& mac) = 0;
		virtual void fillHeader(TxPacketBuilder&, const AddressEthernet& dst, uint16_t etherType) = 0;
	};

    inline Interface(size_t headerSize, AddressResolver* resolver): headerSize(headerSize), resolver(resolver) {}

	inline void ageContent() {}

	inline AddressResolver *getResolver() {
        return resolver;
    }

	inline typename Os::IoChannel *getSender() {
        return static_cast<typename Os::IoChannel*>(this);
    }

    inline size_t getHeaderSize() {
        return headerSize;
    }

    inline void requestRxBuffers(uint16_t n) {
    	this->launch(&Interface::acquireBuffers, n);
	}

    virtual void takeRxBuffers(typename Pool::Allocator allocator) = 0;
private:
    using IoJob = typename Os::IoJob;
	using Result = typename IoJob::Result;
	using Launcher = typename IoJob::Launcher;
	using Callback = typename IoJob::Callback;

	size_t headerSize;
	AddressResolver* const resolver;
    typename Os::template Atomic<uint16_t> nBuffersRequested;

    static inline bool buffersAcquired(Launcher *launcher, IoJob* item, Result result)
    {
    	auto self = static_cast<Interface*>(item);
    	Os::assert(result == Result::Done, NetErrorStrings::unknown);

    	// TODO

    	if(uint16_t n = self->nBuffersRequested.reset()) {
        	typename Pool::Allocator ret = state.pool.template allocateDirect<Pool::Quota::Rx>(n);
        	if(ret.hasMore()) {
        		// TODO
        		return false;
        	}

        	self->request.size = n;
        	self->request.quota = Pool::Quota::Rx;
    		return launcher->launch(&state.pool, &Interface::buffersAcquired, static_cast<typename Pool::IoData*>(self));
    	} else
    		return false;
    }

	static inline void copyBufferAmount(IoJob* item)
	{
		auto self = static_cast<Interface*>(item);
		self->request.size = self->nBuffersRequested.reset();
		self->request.quota = Pool::Quota::Rx;
	}

    static inline bool acquireBuffers(Launcher *launcher, IoJob* item, uint16_t n)
    {
    	typename Pool::Allocator ret = state.pool.template allocateDirect<Pool::Quota::Rx>(n);
    	if(ret.hasMore()) {
    		// TODO
    		return false;
    	}

		auto self = static_cast<Interface*>(item);
    	self->nBuffersRequested.increment(n);
   		return launcher->launch(
   				&state.pool,
   				&Interface::buffersAcquired,
   				static_cast<typename Pool::IoData*>(self),
   				Interface::copyBufferAmount);
    }
};

template<class S, class... Args>
template<uint16_t blockMaxPayload, class... Input>
class Network<S, Args...>::Interfaces<blockMaxPayload, typename NetworkOptions::Set<Input...>, void>:
		public Input::template Wrapped<Network<S, Args...>, blockMaxPayload>...
{
    typedef void (*link)(Interfaces* const ifs);
    static void nop(Interfaces* const ifs) {}

    template<link rest, class C, class...>
	struct Initializer {
		static inline void init(Interfaces* const ifs) {
			static_cast<typename C::template Wrapped<Network<S, Args...>, blockMaxPayload>*>(ifs)->init();
			rest(ifs);
		}

		static constexpr link value = &init;
	};

    template<link rest, class C, class...>
	struct Ager {
		static inline void ageContent(Interfaces* const ifs) {
			static_cast<typename C::template Wrapped<Network<S, Args...>, blockMaxPayload>*>(ifs)->ageContent();
			rest(ifs);
		}

		static constexpr link value = &ageContent;
	};


public:
    inline void init() {
        (pet::ApplyToPack<link, Initializer, &Interfaces::nop, Input...>::value)(this);
	}

	inline void ageContent() {
        (pet::ApplyToPack<link, Ager, &Interfaces::nop, Input...>::value)(this);
	}
};

template<class S, class... Args>
template<template<class, uint16_t> class Driver>
constexpr inline auto* Network<S, Args...>::getEthernetInterface() {
	return static_cast<Ethernet<Driver<Network<S, Args...>, blockMaxPayload>>*>(&state.interfaces);
}

#endif /* INTERFACES_H_ */
