/*
 * PacketProcessor.h
 *
 *  Created on: 2017.12.15.
 *      Author: tooma
 */

#ifndef PACKETPROCESSOR_H_
#define PACKETPROCESSOR_H_

#include "Network.h"

template<class S, class... Args>
class Network<S, Args...>::PacketProcessor: Os::Event {
	using PacketReader = Network<S, Args...>::PacketStream;

	template<class T, void (T::*worker)(Packet)>
	struct Wrapper {
		static inline void trampoline(typename Os::Event* event, uintptr_t arg) {
			Packet chain;
			chain.init(reinterpret_cast<Block*>(arg));
			(static_cast<T*>(static_cast<PacketProcessor*>(event))->*worker)(chain);
		}
	};

protected:
	typedef void (*Callback)(typename Os::Event*, uintptr_t);
	inline PacketProcessor(Callback callback): Os::Event(callback) {}

	template<class T, void (T::*worker)(Packet)>
	static constexpr Callback make() {
		return &Wrapper<T, worker>::trampoline;
	}

public:
	void process(Packet packet) {
		Block* first = packet.first, *last = first;

		while(Block* next = last->getNext())
			last = next;

		Os::submitEvent(this, [first, last](uintptr_t old, uintptr_t& result){
    		if(!old)
    			last->terminate();
			else
    			last->setNext(reinterpret_cast<Block*>(old));

			result = reinterpret_cast<uintptr_t>(first);
			return true;
		});
	}
};



#endif /* PACKETPROCESSOR_H_ */
