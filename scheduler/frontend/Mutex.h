/*
 * Mutex.h
 *
 *  Created on: 2017.05.26.
 *      Author: tooma
 */

#ifndef MUTEX_H_
#define MUTEX_H_

#include "Scheduler.h"

/**
 * Mutex front-end object.
 */
template<class... Args>
class Scheduler<Args...>::Mutex: Policy::Priority, SharedBlocker, Registry<Mutex>::ObjectBase
{
	friend Scheduler<Args...>;
	static constexpr uintptr_t blockedReturnValue = 0;

	typename Policy::Priority &accessPriority() {
		return *static_cast<typename Policy::Priority*>(this);
	}

	Task *owner;
	uintptr_t relockCounter;

	/*
	 * These two methods are never called during normal operation, so
	 * LCOV_EXCL_START is placed here to exclude them from coverage analysis
	 */

	virtual void remove(Blockable*, Blocker*) override final {
		assert(false, "Only priority change can be handled through the Blocker interface of a Mutex");
	}

	/*
	 * From here on, the rest should be check for test coverage, so
	 * LCOV_EXCL_STOP is placed here.
	 */

	virtual Blocker* getTakeable(Task* task) override final {
		return (!owner || owner == task) ? this : nullptr;
	}

	virtual void block(Blockable* b) override final {
		Task* task = b->getTask();
		/*
		 * Raise the priority of the owner to avoid priority inversion.
		 * The original priority will be restored once the mutex is unlocked.
		 */
		if(task->getPriority() < owner->getPriority()) {
			auto oldPrio = owner->getPriority();
			owner->accessPriority() = task->getPriority();

			if(owner->blockedBy)
				owner->blockedBy->priorityChanged(owner, oldPrio);
			else
				{; /* TODO warn that a task sleeps with mutex held.*/ }
		}

		SharedBlocker::block(b);
	}

	virtual uintptr_t acquire(Task* task) override final {
		if(!owner) {
			owner = task;
			/*
			 * Save original priority now, so that it can be restored if
			 * a later lock operation issued by a higher priority task
			 * raises the priority of the _currentTask_ through the
			 * ownership relation.
			 */
			accessPriority() = task->getPriority();
		} else {
			relockCounter++;
		}

		return relockCounter;
	}

	virtual bool release(uintptr_t arg) override {
		Task* currentTask = getCurrentTask();

		assert(currentTask == owner, "Mutex unlock from non-owner task");
		assert(!arg, "Asynchronous mutex unlock");

		if(relockCounter)
			relockCounter--;
		else {
			/*
			 * Restore priority. Here the _currentTask_ is executing, so it
			 * is sure to not be handled by the policy currently, so the
			 * priority can be set directly.
			 */
			currentTask->accessPriority() = this->accessPriority();

			/*
			 * We know that the sleeper obtained here is actually a task,
			 * because only the block method can add elements to the
			 * waiter list, and that only accepts tasks.
			 *
			 * We also know that the task is not queued for a timeout,
			 * because locking with timeout is not supported (on purpose).
			 */
			if(Task* waken = static_cast<Task*>(this->waiters.popLowest())) {
				waken->blockedBy = nullptr;
				owner = waken;

				state.policy.addRunnable(waken);

				return waken->getPriority() < currentTask->getPriority();
			} else {
				owner = nullptr;
			}
		}

		return false;
	}

public:
	inline void init() {
		owner = nullptr;
		relockCounter = 0;
		Registry<Mutex>::registerObject(this);
	}

	inline void lock() {
		syscall(&Scheduler<Args...>::doBlock<Mutex>, Registry<Mutex>::getRegisteredId(this));
	}

	inline void unlock() {
		syscall(&Scheduler<Args...>::doRelease<Mutex>, Registry<Mutex>::getRegisteredId(this));
	}

	inline ~Mutex() {
		Registry<Mutex>::unregisterObject(this);
	}
};

#endif /* MUTEX_H_ */
