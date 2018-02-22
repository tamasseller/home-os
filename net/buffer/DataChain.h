/*
 * DataChain.h
 *
 *  Created on: 2018.01.20.
 *      Author: tooma
 */

#ifndef DATACHAIN_H_
#define DATACHAIN_H_

template<class S, class... Args>
struct Network<S, Args...>::DataChain: Chain {
	class Packetizatation: public Packet {
		friend DataChain;
		Block* last;
		uint32_t savedLength;
	};

	bool packetize(Packetizatation& p, uint16_t length)
	{
		if(!this->head)
			return false;

		p.init(this->head);
		Block* last = this->head;

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
			this->head = last;
		} else {
			this->head = last->getNext();
			if(!this->head)
				this->tail = nullptr;
		}

		if(first != this->head) {
			first->cleanup();
			typename Pool::Deallocator deallocator(first);

	        for(Block* current = first->getNext(); current != this->head;) {
	            Block* next = current->getNext();

	            current->cleanup();
                deallocator.take(current);

	            current = next;
	        }

	        deallocator.template deallocate<quota>(&state.pool);
		}
	}

	inline void sweepOutTo(DataChain& other)
	{
		if(this->head) {
			if(other.tail)
				other.tail->setNext(this->head);
			else
				other.head = this->head;

			other.tail = this->tail;

			this->tail = this->head = nullptr;
		}
	}

	inline void put(Packet packet)
	{
		Block *first = Packet::Accessor::getFirst(packet);
		Block *last = first->findEndOfPacket();
		last->clearEndOfPacket();
		Chain::put(first, last);
	}

	inline Block* get()
	{
		if(this->head) {
			Block* ret = this->head;

			if(this->head == this->tail)
				this->head = this->tail = nullptr;
			else
				this->head = this->head->getNext();

			return ret;
		}

		return nullptr;
	}

	inline bool isEmpty() const {
		return this->head == nullptr;
	}
};

#endif /* DATACHAIN_H_ */
