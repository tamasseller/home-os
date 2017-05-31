/*
 * Waitable.h
 *
 *  Created on: 2017.05.30.
 *      Author: tooma
 */

#ifndef WAITABLE_H_
#define WAITABLE_H_

#include "Scheduler.h"

template<class... Args>
template<class Child>
class Scheduler<Args...>::
Waitable: public Event<Child> {
	friend Scheduler<Args...>;
public:
	void init();
	void wait(uintptr_t timeout);
	void notify();

	using Combiner = typename Child::Combiner;
};


#endif /* WAITABLE_H_ */
