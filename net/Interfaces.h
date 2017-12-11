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
class Network<S, Args...>::Interface: public Os::IoChannel {
	friend class Network<S, Args...>;

public:
	struct AddressResolver: Os::IoChannel {
		virtual const AddressEthernet& getAddress() = 0;
		virtual bool resolveAddress(AddressIp4 ip, AddressEthernet& mac) = 0;
		virtual bool fillHeader(TxPacketBuilder&, const AddressEthernet& dst, uint16_t etherType) = 0;
	};

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

    inline Interface(size_t headerSize, AddressResolver* resolver): headerSize(headerSize), resolver(resolver) {}

private:

	const size_t headerSize;
	AddressResolver* const resolver;
};

template<class S, class... Args>
template<class... Input>
class Network<S, Args...>::Interfaces<typename NetworkOptions::Set<Input...>, void>: public Input::template Wrapped<Network<S, Args...>>... {
    typedef void (*link)(Interfaces* const ifs);
    static void nop(Interfaces* const ifs) {}

    template<link rest, class C, class...>
	struct Initializer {
		static inline void init(Interfaces* const ifs) {
			static_cast<typename C::template Wrapped<Network<S, Args...>>*>(ifs)->init();
			rest(ifs);
		}

		static constexpr link value = &init;
	};

    template<link rest, class C, class...>
	struct Ager {
		static inline void ageContent(Interfaces* const ifs) {
			static_cast<typename C::template Wrapped<Network<S, Args...>>*>(ifs)->ageContent();
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
template<template<class> class Driver>
constexpr inline typename Network<S, Args...>::template Ethernet<Driver<Network<S, Args...>>> *
Network<S, Args...>::geEthernetInterface() {
	return static_cast<Ethernet<Driver<Network<S, Args...>>>*>(&state.interfaces);
}

#endif /* INTERFACES_H_ */
