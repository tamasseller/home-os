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
class Network<S, Args...>::Interface: public Os::IoChannelCommon {
	friend class Network<S, Args...>;

	const size_t headerSize;

public:
	virtual bool resolveAddress(AddressIp4 ip, AddressEthernet& mac) = 0;
	virtual bool requestResolution(typename Os::IoJob::Hook, typename Os::IoJob*, typename Os::IoJob::Callback, ArpTableIoData*) = 0;
	virtual bool fillHeader(TxPacketBuilder&, const AddressEthernet& dst, uint16_t etherType) = 0;

    virtual const AddressEthernet& getAddress() = 0;

	inline typename Os::IoChannel *getSender() {
        return static_cast<typename Os::IoChannel*>(this);
    }

    inline size_t getHeaderSize() {
        return headerSize;
    }

    inline Interface(size_t headerSize): headerSize(headerSize) {}
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

public:
	void init() {
        (pet::ApplyToPack<link, Initializer, &Interfaces::nop, Input...>::value)(this);
	}
};

template<class S, class... Args>
template<class C>
constexpr inline typename Network<S, Args...>::template Ethernet<C> *Network<S, Args...>::geEthernetInterface() {
	return static_cast<Ethernet<C>*>(&state.interfaces);
}

#endif /* INTERFACES_H_ */
