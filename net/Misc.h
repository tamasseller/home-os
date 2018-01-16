/*
 * Misc.h
 *
 *  Created on: 2018.01.16.
 *      Author: tooma
 */

#ifndef MISC_H_
#define MISC_H_

#include "Network.h"

template<class S, class... Args>
struct Network<S, Args...>::BufferStats {
	size_t nBuffersUsed, nTxUsed, nRxUsed;
};

template<class S, class... Args>
inline typename Network<S, Args...>::BufferStats Network<S, Args...>::getBufferStats() {
	BufferStats ret;

	ret.nBuffersUsed = state.pool.statUsed();
	ret.nTxUsed = state.pool.statTxUsed();
	ret.nRxUsed = state.pool.statRxUsed();

	return ret;
}

template<class S, class... Args>
template<bool enabled>
inline typename EnableIf<enabled, DiagnosticCounters>::Type Network<S, Args...>::getCounterStats() {
	return state;
}

template<class S, class... Args>
inline void Network<S, Args...>::init(Buffers &buffers) {
    state.~State();
    new(&state) State();
    state.rawInputChannel.init();
    state.icmpInputChannel.init();
    state.udpInputChannel.init();
    state.tcpInputChannel.init();
    state.tcpListenerChannel.init();
	state.pool.init(buffers);
	state.interfaces.init();
	state.ager.start(secTicks);
}

template<class S, class... Args>
inline bool Network<S, Args...>::addRoute(const Route& route, bool setUp) {
    return state.routingTable.add(route, setUp);
}

template<class S, class... Args>
inline void Network<S, Args...>::State::Ager::age() {
	state.interfaces.ageContent();
	this->extend(secTicks);
}

template<class S, class... Args>
inline void Network<S, Args...>::State::Ager::doAgeing(typename Os::Sleeper* sleeper) {
	static_cast<Ager*>(sleeper)->age();
}

template<class S, class... Args>
inline constexpr uint16_t Network<S, Args...>::bytesToBlocks(size_t bytes) {
	return static_cast<uint16_t>((bytes + blockMaxPayload - 1) / blockMaxPayload);
}

template<class S, class... Args>
struct Network<S, Args...>::RxPacketHandler {
	virtual void handlePacket(Packet packet) = 0;
};

#endif /* MISC_H_ */
