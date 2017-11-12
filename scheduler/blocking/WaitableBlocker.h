/*
 * WaitableBlocker.h
 *
 *  Created on: 2017.11.08.
 *      Author: tooma
 */

#ifndef WAITABLEBLOCKER_H_
#define WAITABLEBLOCKER_H_

#include "Scheduler.h"

template<class... Args>
template<class Semaphore>
struct Scheduler<Args...>::WaitableBlocker
{
	inline void wait() {
		Scheduler<Args...>::blockOn(static_cast<Semaphore*>(this));
	}

	inline bool wait(uintptr_t timeout) {
		return Scheduler<Args...>::timedBlockOn(static_cast<Semaphore*>(this), timeout);
	}
};

#endif /* WAITABLEBLOCKER_H_ */
