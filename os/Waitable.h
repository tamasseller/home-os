/*
 * Waitable.h
 *
 *  Created on: 2017.05.30.
 *      Author: tooma
 */

#ifndef WAITABLE_H_
#define WAITABLE_H_

#include "Scheduler.h"

#include "data/LinkedList.h"

template<class... Args>
class Scheduler<Args...>::Waitable: Scheduler<Args...>::Mutex, public Event {
	friend Scheduler<Args...>;

	virtual uintptr_t acquire(uintptr_t) = 0;
	virtual uintptr_t release(uintptr_t) = 0;

public:
	void init();
	void wait(uintptr_t timeout);
	void notify();
};


#endif /* WAITABLE_H_ */
