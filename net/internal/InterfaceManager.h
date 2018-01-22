/*
 * InterfaceManager.h
 *
 *  Created on: 2017.11.21.
 *      Author: tooma
 */

#ifndef INTERFACEMANAGER_H_
#define INTERFACEMANAGER_H_

#include "Network.h"

#include "meta/ApplyToPack.h"

template<class S, class... Args>
template<uint16_t blockMaxPayload, class... Input>
class Network<S, Args...>::InterfaceManager<blockMaxPayload, typename NetworkOptions::Set<Input...>, void>:
		public Input::template Wrapped<Network<S, Args...>, blockMaxPayload>...
{
    typedef void (*link)(InterfaceManager* const ifs);
    static void nop(InterfaceManager* const ifs) {}

    template<link rest, class C, class...>
	struct Initializer {
		static inline void init(InterfaceManager* const ifs) {
			static_cast<typename C::template Wrapped<Network<S, Args...>, blockMaxPayload>*>(ifs)->init();
			rest(ifs);
		}

		static constexpr link value = &init;
	};

    template<link rest, class C, class...>
	struct Ager {
		static inline void ageContent(InterfaceManager* const ifs) {
			static_cast<typename C::template Wrapped<Network<S, Args...>, blockMaxPayload>*>(ifs)->ageContent();
			rest(ifs);
		}

		static constexpr link value = &ageContent;
	};

public:
    inline void init() {
        (pet::ApplyToPack<link, Initializer, &InterfaceManager::nop, Input...>::value)(this);
	}

	inline void ageContent() {
        (pet::ApplyToPack<link, Ager, &InterfaceManager::nop, Input...>::value)(this);
	}
};

template<class S, class... Args>
template<template<class, uint16_t> class Driver>
constexpr inline auto* Network<S, Args...>::getEthernetInterface() {
	return static_cast<Ethernet<Driver<Network<S, Args...>, blockMaxPayload>>*>(&state.interfaces);
}

#endif /* INTERFACEMANAGER_H_ */
