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
class Scheduler<Args...>::Mutex: Policy::Priority, SharedBlocker
{
	friend Scheduler<Args...>;
	static constexpr uintptr_t blockedReturnValue = 0;

	static bool comparePriority(const Blockable& a, const Blockable& b) {
		return firstPreemptsSecond(static_cast<const Task*>(&a), static_cast<const Task*>(&b));
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
		if(firstPreemptsSecond(task, owner)) {
			Priority oldPrio = *owner;
			*static_cast<typename Policy::Priority*>(owner) = *task;

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
			*static_cast<typename Policy::Priority*>(this) = *task;
		} else {
			relockCounter++;
		}

		return relockCounter;
	}

	virtual bool release(uintptr_t arg) override {
		Task* currentTask = static_cast<Task*>(Profile::getCurrent());

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
			*static_cast<typename Policy::Priority*>(currentTask) = *this;

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

				return firstPreemptsSecond(waken, currentTask);
			} else {
				owner = nullptr;
			}
		}

		return false;
	}

public:
	void init() {
		owner = nullptr;
		relockCounter = 0;
	}

	void lock() {
		Profile::sync(&Scheduler<Args...>::doBlock<Mutex>, detypePtr(this));
	}

	void unlock() {
		Profile::sync(&Scheduler<Args...>::doRelease<Mutex>, detypePtr(this));
	}
};

#endif /* MUTEX_H_ */
