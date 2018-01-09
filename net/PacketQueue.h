/*
 * PacketQueue.h
 *
 *  Created on: 2017.12.20.
 *      Author: tooma
 */

#ifndef PACKETQUEUE_H_
#define PACKETQUEUE_H_

template<class S, class... Args>
class Network<S, Args...>::PacketQueue
{
    typename Os::template Atomic<Block*> head;

public:
    inline void putPacketChain(Packet packet) {
		Block* first = packet.first, *last = first;

		while(Block* next = last->getNext())
			last = next;

    	this->head([](Block* old, Block*& result, Block* first, Block* last){
    		if(!old)
    			last->terminate();
			else
    			last->setNext(old);
			result = first;
			return true;
		}, first, last);
    }

    inline bool takePacketFromQueue(Packet &ret) {
    	if(Block* first = this->head.reset()) {
    		ret.init(first);

    		while(!first->isEndOfPacket()) {
    			first = first->getNext();
    			NET_ASSERT(first);
    		}

    		if(Block* next = first->getNext()) {
    			Packet remaining;
    			remaining.init(next);
    			this->putPacketChain(remaining);
    		}

    		first->terminate();
    		return true;
    	}

    	return false;
	}

    inline bool isEmpty() {
    	return !head;
    }
};


#endif /* PACKETQUEUE_H_ */
