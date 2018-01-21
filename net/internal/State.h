/*
 * State.h
 *
 *  Created on: 2018.01.21.
 *      Author: tooma
 */

#ifndef STATE_H_
#define STATE_H_

#include "Network.h"

template<class S, class... Args>
typename Network<S, Args...>::State NetworkOptions::Configurable<S, Args...>::state;

template<class S, class... Args>
struct Network<S, Args...>::State: DiagnosticCounterStorage<useDiagnosticCounters> {
	struct Ager: Os::Timeout {
		static inline void doAgeing(typename Os::Sleeper* sleeper) {
			state.interfaces.ageContent();
			static_cast<Ager*>(sleeper)->extend(secTicks);
		}
		inline Ager(): Os::Timeout(&Ager::doAgeing) {}
	} ager;

    inline void* operator new(size_t, void* x) { return x; }

    PacketProcessor rxProcessor;

    RawCore rawCore;
    IcmpCore icmpCore;
    UdpCore udpCore;
    TcpCore tcpCore;

	Interfaces<Block::dataSize, IfsToBeUsed> interfaces;
	RoutingTable routingTable;
	Pool pool;

	inline State():
		rxProcessor(PacketProcessor::template makeStatic<&Network<S, Args...>::processReceivedPacket>()) {}
};

#endif /* STATE_H_ */
