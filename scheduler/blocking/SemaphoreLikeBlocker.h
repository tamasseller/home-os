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

			waken->blockedBy->canceled(waken, this);

			state.policy.addRunnable(waken);

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

public:
	inline void wait() {
		auto id = Scheduler<Args...>::template Registry<Semaphore>::getRegisteredId(static_cast<Semaphore*>(this));
		syscall<SYSCALL(doBlock<Semaphore>)>(id);
	}

	inline bool wait(uintptr_t timeout) {
		auto id = Scheduler<Args...>::template Registry<Semaphore>::getRegisteredId(static_cast<Semaphore*>(this));
		return syscall<SYSCALL(doTimedBlock<Semaphore>)>(id, timeout);
	}
};

#endif /* SEMAPHORELIKEBLOCKER_H_ */
