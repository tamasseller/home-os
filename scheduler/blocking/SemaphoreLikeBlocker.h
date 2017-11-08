/*
 * SemaphoreLikeBlocker.h
 *
 *  Created on: 2017.10.25.
 *      Author: tooma
 */

#ifndef SEMAPHORELIKEBLOCKER_H_
#define SEMAPHORELIKEBLOCKER_H_

#include "Scheduler.h"

template<class... Args>
template<class Semaphore>
class Scheduler<Args...>::SemaphoreLikeBlocker:
		public SharedBlocker,
		public WaitableBlocker<Semaphore>,
		public AsyncBlocker<Semaphore>
{
	friend class Scheduler<Args...>;
	static constexpr uintptr_t timeoutReturnValue = 0;
	static constexpr uintptr_t blockedReturnValue = 1;

protected:
	inline bool wakeOne()
	{
		if(Blockable* blockable = this->waiters.lowest()) {
			blockable->wake(this);
			return true;
		}

		return false;
	}

	virtual void canceled(Blockable* ba, Blocker* be) final override {
		this->waiters.remove(ba);
	}

	virtual void timedOut(Blockable* ba) final override {
		this->waiters.remove(ba);
		Profile::injectReturnValue(ba->getTask(), Semaphore::timeoutReturnValue);
	}
};

#endif /* SEMAPHORELIKEBLOCKER_H_ */
