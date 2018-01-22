/*
 * Packet.h
 *
 *  Created on: 2017.11.19.
 *      Author: tooma
 */

#ifndef PACKET_H_
#define PACKET_H_

#include "Network.h"

template<class S, class... Args>
class Network<S, Args...>::Packet
{
	Block* first;

public:
	struct Accessor {
		static constexpr inline Block* getFirst(const Packet& p) {
			return p.first;
		}
	};

	inline Packet(Block* first): first(first) {}
	inline Packet() = default;

	inline void init(Block* first) {
		this->first = first;
	}

	inline bool isValid() {
		return this->first != nullptr;
	}

	template<typename Pool::Quota quota>
	void dispose() {
		if(!first)
			return;

        typename Pool::Deallocator deallocator(first);

        for(Block* current = first->getNext(); current;) {
            Block* next = current->getNext();

            current->cleanup();

            if(current->isEndOfPacket()) {
                deallocator.take(current);
                break;
            } else
                deallocator.take(current);

            current = next;
        }

        deallocator.template deallocate<quota>(&state.pool);
	}

    /**
     * Increase data offset at the start of the packet.
     *
     * If block becomes empty it is dropped.
     */
	template<typename Pool::Quota quota>
    bool dropInitialBytes(size_t n) {
        while(n) {
        	auto size = first->getSize();
            if(n < size) {
            	first->increaseOffset(n);
                return true;
            } else {
                Block* current = first->getNext();
                n -= size;

                first->cleanup();
                typename Pool::Deallocator deallocator(first);

                while(current) {
                	auto size = current->getSize();
                	if(n < size)
                		break;

                    Block* next = current->getNext();

                    n -= size;
                    current->cleanup();
                    deallocator.take(current);

                    current = next;
                }

                deallocator.template deallocate<quota>(&state.pool);

                if((first = current) == nullptr)
                    return false;
            }
        }

        return true;
    }

	bool operator ==(const Packet& other) {
		return first == other.first;
	}
};

#endif /* PACKET_H_ */
