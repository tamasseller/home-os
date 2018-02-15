/*
 * Chain.h
 *
 *  Created on: 2018.02.14.
 *      Author: tooma
 */

#ifndef CHAIN_H_
#define CHAIN_H_

template<class S, class... Args>
class Network<S, Args...>::Chain {
protected:
	Block *head, *tail;

	inline void put(Block *first, Block *last)
	{
		last->terminate();

		if(head)
			tail->setNext(first);
		else
			head = first;

		tail = last;
	}

public:
	inline Chain(): head(nullptr), tail(nullptr) {}

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
};

#endif /* CHAIN_H_ */
