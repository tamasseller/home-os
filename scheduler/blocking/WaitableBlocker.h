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
		auto id = Scheduler<Args...>::template Registry<Semaphore>::getRegisteredId(static_cast<Semaphore*>(this));
		syscall<SYSCALL(doBlock<Semaphore>)>(id);
	}

	inline bool wait(uintptr_t timeout) {
		auto id = Scheduler<Args...>::template Registry<Semaphore>::getRegisteredId(static_cast<Semaphore*>(this));
		return syscall<SYSCALL(doTimedBlock<Semaphore>)>(id, timeout);
	}
};

#endif /* WAITABLEBLOCKER_H_ */
