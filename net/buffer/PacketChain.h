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

	bool peek(Packet &ret)
	{
		if(!head)
			return false;

		ret.init(head);
		return true;
	}

	bool take(Packet &ret)
	{
		if(!peek(ret))
			return false;

		Block *last = head->findEndOfPacket();

		if(!(head = last->getNext()))
			tail = nullptr;

		last->terminate();
		return true;
	}

	void put(Packet packet)
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

	template<typename Pool::Quota quota>
	void dropAll() {
		if(head) {
			typename Pool::Deallocator deallocator(head);

	        for(Block* current = head->getNext(); current;) {
	            Block* next = current->getNext();

	            current->cleanup();
                deallocator.take(current);

	            current = next;
	        }

			deallocator.template deallocate<quota>(&state.pool);

			head = tail = nullptr;
		}
	}

	inline void reset() {
		head = nullptr;
	}

	inline bool isEmpty() {
		return head == nullptr;
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
