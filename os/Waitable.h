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
class Scheduler<Args...>::Waitable: public Event {
	friend Scheduler<Args...>;

	inline void interrupted() {}
public:
	void init();
	void wait(uintptr_t timeout);
	void notify();
};


#endif /* WAITABLE_H_ */
