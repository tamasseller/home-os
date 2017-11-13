/*******************************************************************************
 *
 * Copyright (c) 2017 Seller Tamás. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *******************************************************************************/

#ifndef MUTEX_H_
#define MUTEX_H_

#include "Scheduler.h"

namespace home {

/**
 * Mutex front-end object.
 */
template<class... Args>
class Scheduler<Args...>::Mutex: Policy::Priority, SharedBlocker, Registry<Mutex>::ObjectBase
{
	friend Scheduler<Args...>;
	static constexpr uintptr_t blockedReturnValue = 0;

	template<bool, class=void> class DeadlockDetectionSwitch;

	typename Policy::Priority &accessPriority() {
		return *static_cast<typename Policy::Priority*>(this);
	}

	Task *owner = nullptr;
	uintptr_t relockCounter = 0;

	/*
	 * This method is never called during normal operation, so
	 * LCOV_EXCL_START is placed here to exclude them from coverage analysis
	 */

	virtual void canceled(Blockable*, Blocker*) override final {
		assert(false, ErrorStrings::mutexBlockerUsage);
	}

	virtual void timedOut(Blockable*) override final {
		assert(false, ErrorStrings::mutexBlockerUsage);
	}

	/*
	 * From here on, the rest should be check for test coverage, so
	 * LCOV_EXCL_STOP is placed here.
	 */

	virtual Blocker* getTakeable(Task* task) override final
	{
		assert(!DeadlockDetectionSwitch<deadlockDetectionEnabled>::detectDeadlock(this, task),
				ErrorStrings::mutexDeadlock);

		return (!owner || owner == task) ? this : nullptr;
	}

	virtual Task* getOwner() override final {
		return owner;
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

		assert(currentTask == owner, ErrorStrings::mutexNonOwnerUnlock);
		assert(!arg, ErrorStrings::mutexAsyncUnlock);

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

				return true;
			} else {
				owner = nullptr;
			}
		}

		return false;
	}

	void* operator new(size_t, void* r) {return r;}

public:
	inline void init() {
		resetObject(this);
		Registry<Mutex>::registerObject(this);
	}

	inline void lock() {
		blockOn(this);
	}

	inline void unlock() {
		syscall<SYSCALL(doRelease<Mutex>)>(Registry<Mutex>::getRegisteredId(this), (uintptr_t)0);
	}

	inline ~Mutex() {
		Registry<Mutex>::unregisterObject(this);
	}
};

template<class... Args>
template<class Dummy>
struct Scheduler<Args...>::Mutex::DeadlockDetectionSwitch<false, Dummy> {
	static bool detectDeadlock(Blocker* blocker, Task* task) {
		return false;
	}
};

template<class... Args>
template<class Dummy>
struct Scheduler<Args...>::Mutex::DeadlockDetectionSwitch<true, Dummy> {
	static bool detectDeadlock(Blocker* blocker, Task* checkedTask)
	{
		if(blocker->getOwner() == checkedTask)
			return false;

		while(blocker) {
			if(Task* otherTask = blocker->getOwner()) {
				if(otherTask == checkedTask)
					return true;

				blocker = otherTask->blockedBy;
			} else {
				break;
			}
		}

		return false;
	}
};

}

#endif /* MUTEX_H_ */
