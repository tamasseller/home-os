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
class Network<S, Args...>::PacketChain {
	Block *head, *tail;

public:
	inline PacketChain(): head(nullptr), tail(nullptr) {}

	inline bool take(Packet &ret)
	{
		if(!head)
			return false;

		ret.init(head);

		Block *last = head->findEndOfPacket();
		head = last->findEndOfPacket()->getNext();
		last->terminate();

		if(!head)
			tail = nullptr;

		return true;
	}

	inline void put(Packet packet)
	{
		Block *first = Packet::Accessor::getFirst(packet);
		Block *last = first->findEndOfPacket();
		last->terminate();

		if(head)
			tail->setNext(first);
		else
			head = first;

		tail = last;
	}

	inline void reset() {
		head = nullptr;
	}

	inline bool isEmpty() {
		return head == nullptr;
	}

	inline static PacketChain flip(Block* first) {
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
