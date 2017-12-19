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

	template<class T, void (T::*worker)(PacketReader)>
	struct Wrapper {
		static inline void trampoline(typename Os::Event* event, uintptr_t arg) {
			Packet packet;
			packet.init(reinterpret_cast<Block*>(arg));
			PacketReader reader(packet);

			do {
				(static_cast<T*>(event)->*worker)(reader);
			} while(reader.moveToNextPacket());
		}
	};

protected:
	inline PacketProcessor(void (*callback)(typename Os::Event*, uintptr_t)): Os::Event(callback) {}

	template<class T, void (T::*worker)(PacketReader)>
	static constexpr void (*make(typename Os::Event*, uintptr_t)) () {
		return &Wrapper<T, worker>::trampoline;
	}

public:
	void process(Packet packet) {
		Block* first = packet.first, *last = first;

		while(Block* next = last->getNext())
			last = next;

		Os::submitEvent(this, [first, last](uintptr_t old, uintptr_t& result){
			last->setNext(reinterpret_cast<Block*>(old));
			result = reinterpret_cast<uintptr_t>(first);
			return true;
		});
	}
};

template<class S, class... Args>
class Network<S, Args...>::IpPacketProcessor: PacketProcessor {};

#endif /* PACKETPROCESSOR_H_ */
