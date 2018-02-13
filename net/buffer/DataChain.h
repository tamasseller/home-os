/*
 * DataChain.h
 *
 *  Created on: 2018.01.20.
 *      Author: tooma
 */

#ifndef DATACHAIN_H_
#define DATACHAIN_H_

template<class S, class... Args>
class Network<S, Args...>::DataChain {
	Block *head, *tail;

public:
	inline DataChain(): head(nullptr), tail(nullptr) {}

	class Packetizatation: public Packet {
		friend DataChain;
		Block* last;
		uint32_t savedLength;
	};

	bool packetize(Packetizatation& p, uint16_t length)
	{
		if(!head)
			return false;

		p.init(head);
		Block* last = head;

		while(true) {
			auto size = last->getSize();

			if(length <= size)
				break;

			if(Block *next = last->getNext()) {
				length = static_cast<uint16_t>(length - size);
				last = next;
			} else {
				return false;
			}
		}

		last->setEndOfPacket();
		p.savedLength = last->getSize();
		last->setSize(length);
		p.last = last;

		return true;
	}

	void revert(const Packetizatation& p) {
		Block* last = p.last;
		last->setSize(p.savedLength);
		last->clearEndOfPacket();
	}

	template<typename Pool::Quota quota>
	void apply(const Packetizatation& p)
	{
		Block* last = p.last;
		Block* first = Packet::Accessor::getFirst(p);

		auto offset = last->getSize();

		if(offset != p.savedLength) {
			revert(p);
			last->increaseOffset(offset);
			head = last;
		} else {
			head = last->getNext();
			if(!head)
				tail = nullptr;
		}

		if(first != head) {
			first->cleanup();
			typename Pool::Deallocator deallocator(first);

	        for(Block* current = first->getNext(); current != head;) {
	            Block* next = current->getNext();

	            current->cleanup();
                deallocator.take(current);

	            current = next;
	        }

	        deallocator.template deallocate<quota>(&state.pool);
		}
	}

	inline void put(Packet packet)
	{
		Block *first = Packet::Accessor::getFirst(packet);
		Block *last = first->findEndOfPacket();
		last->terminate();
		last->clearEndOfPacket();

		if(head)
			tail->setNext(first);
		else
			head = first;

		tail = last;
	}

};

#endif /* DATACHAIN_H_ */
