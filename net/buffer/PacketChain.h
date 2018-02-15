/*
 * PacketChain.h
 *
 *  Created on: 2018.01.20.
 *      Author: tooma
 */

#ifndef PACKETCHAIN_H_
#define PACKETCHAIN_H_

#include "Network.h"

template<class S, class... Args>
struct Network<S, Args...>::PacketChain: Chain {
	bool peek(Packet &ret)
	{
		if(!this->head)
			return false;

		ret.init(this->head);
		return true;
	}

	bool take(Packet &ret)
	{
		if(!peek(ret))
			return false;

		Block *last = this->head->findEndOfPacket();

		if(!(this->head = last->getNext()))
			this->tail = nullptr;

		last->terminate();
		return true;
	}

	void put(Packet packet)
	{
		Block *first = Packet::Accessor::getFirst(packet);
		Block *last = first->findEndOfPacket();
		Chain::put(first, last);
	}

	inline void reset() {
		this->head = nullptr;
	}

	inline bool isEmpty() {
		return this->head == nullptr;
	}

	static PacketChain flip(Block* first) {
		PacketChain ret;

		Block *last = first->findEndOfPacket();
		ret.tail = last;

		Block *current = last->getNext();
		last->terminate();

		Block* prev = first;
		while(current) {
			Block *last = current->findEndOfPacket();
			Block* oldNext = last->getNext();
			last->setNext(prev);
			prev = current;
			current = oldNext;
		}

		ret.head = prev;
		return ret;
	}
};

#endif /* PACKETCHAIN_H_ */
