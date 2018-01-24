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

	template<class C>
	inline void process(uintptr_t arg, C&& c)
	{
        for(PacketChain chain = PacketChain::flip(reinterpret_cast<Block*>(arg)); !chain.isEmpty();) {
            Packet p;
            chain.take(p);
            c(p);
        };
	}

	template<class T, void (T::*worker)(Packet)>
	struct Wrapper {
		static inline void trampoline(typename Os::Event* event, uintptr_t arg) {
			static_cast<PacketProcessor*>(event)->process(arg, [event](Packet packet){
				(static_cast<T*>(static_cast<PacketProcessor*>(event))->*worker)(packet);
			});
		}
	};

	template<void (*worker)(Packet)>
	struct StaticWrapper {
		static inline void trampoline(typename Os::Event* event, uintptr_t arg) {
			static_cast<PacketProcessor*>(event)->process(arg, [event](Packet packet){
				(*worker)(packet);
			});
		}
	};

protected:
	typedef void (*Callback)(typename Os::Event*, uintptr_t);

	template<class T, void (T::*worker)(Packet)>
	static constexpr Callback make() {
		return &Wrapper<T, worker>::trampoline;
	}

	template<void (*worker)(Packet)>
	static constexpr Callback makeStatic() {
		return &StaticWrapper<worker>::trampoline;
	}


public:
	inline PacketProcessor(Callback callback): Os::Event(callback) {}

	void process(Packet packet) {
		Block* first = Packet::Accessor::getFirst(packet);
		Block* last = first;

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
