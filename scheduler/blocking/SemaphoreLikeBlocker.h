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
class Scheduler<Args...>::SemaphoreLikeBlocker: public SharedBlocker, public AsyncBlocker<Semaphore> {
	friend class Scheduler<Args...>;
	static constexpr uintptr_t timeoutReturnValue = 0;
	static constexpr uintptr_t blockedReturnValue = 1;

protected:
	inline bool wakeOne()
	{
		if(Blockable* blockable = this->waiters.lowest()) {
			Task* waken = blockable->getTask();

			if(waken->isSleeping())
				state.sleepList.remove(waken);

			waken->blockedBy->remove(waken, this);

			state.policy.addRunnable(waken);

			return true;
		}

		return false;
	}

	virtual void remove(Blockable* ba, Blocker* be) final override {
		this->waiters.remove(ba);

		if(!be)
			Profile::injectReturnValue(ba->getTask(), false);
	}

public:
	inline void wait() {
		Profile::sync(&Scheduler<Args...>::doBlock<Semaphore>, detypePtr(this));
	}

	inline bool wait(uintptr_t timeout) {
		return Profile::sync(&Scheduler<Args...>::doTimedBlock<Semaphore>, detypePtr(this), timeout);
	}
};

#endif /* SEMAPHORELIKEBLOCKER_H_ */
