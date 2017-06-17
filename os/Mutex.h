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
class Scheduler<Args...>::Mutex: Policy::Priority, Blocker
{
	friend Scheduler<Args...>;

	static bool comparePriority(const Blockable& a, const Blockable& b) {
		return firstPreemptsSecond(static_cast<const Task*>(&a), static_cast<const Task*>(&b));
	}

	Task *owner;
	pet::OrderedDoubleList<Blockable, &Mutex::comparePriority> waiters;
	uintptr_t relockCounter;

	/*
	 * These two methods are never called during normal operation, so
	 * LCOV_EXCL_START is placed here to exclude them from coverage analysis
	 */

	virtual void remove(Task*) {
		assert(false, "Only priority change can be handled through the Blocker interface of a Mutex");
	}

	virtual void waken(Task*, Waitable*) {
		assert(false, "Only priority change can be handled through the Blocker interface of a Mutex");
	}

	/*
	 * From here on, the rest should be check for test coverage, so
	 * LCOV_EXCL_STOP is placed here.
	 */

	virtual void priorityChanged(Task* task, Priority old) {
		waiters.remove(task);
		waiters.add(task);
	}

public:
	void init() {
		owner = nullptr;
	}

	void lock() {
		Profile::sync(&Scheduler<Args...>::doLock, detypePtr(this));
	}

	void unlock() {
		Profile::sync(&Scheduler<Args...>::doUnlock, detypePtr(this));
	}
};

template<class... Args>
uintptr_t Scheduler<Args...>::
doLock(uintptr_t mutexPtr)
{
	Mutex* mutex = entypePtr<Mutex>(mutexPtr);
	Task* currentTask = static_cast<Task*>(Profile::getCurrent());

	if(!mutex->owner) {
		mutex->owner = currentTask;
		mutex->relockCounter = 0;

		assert(!mutex->waiters.lowest(), "Mutex waiter stuck");

		/*
		 * Save original priority now, so that it can be restored if
		 * a later lock operation issued by a higher priority task
		 * raises the priority of the _currentTask_ through the
		 * ownership relation.
		 */
		*static_cast<typename Policy::Priority*>(mutex) = *currentTask;

	} else if(mutex->owner == currentTask) {
		mutex->relockCounter++;
	} else {
		/*
		 * Raise the priority of the owner to avoid priority inversion.
		 * The original priority will be restored once the mutex is unlocked.
		 */
		if(firstPreemptsSecond(currentTask, mutex->owner)) {
			Priority oldPrio = *mutex->owner;
			*static_cast<typename Policy::Priority*>(mutex->owner) = *currentTask;

			if(mutex->owner->blockedBy)
				mutex->owner->blockedBy->priorityChanged(mutex->owner, oldPrio);
		}

		currentTask->blockedBy = mutex;
		mutex->waiters.add(currentTask);
		switchToNext<false>();
	}

	return true;
}

template<class... Args>
uintptr_t Scheduler<Args...>::
doUnlock(uintptr_t mutexPtr)
{
	Mutex* mutex = entypePtr<Mutex>(mutexPtr);
	Task* currentTask = static_cast<Task*>(Profile::getCurrent());

	assert(currentTask == mutex->owner, "Mutex unlock from non-owner task");

	if(mutex->relockCounter)
		mutex->relockCounter--;
	else {
		/*
		 * Restore priority. Here the _currentTask_ is executing, so it
		 * is sure to not be handled by the policy currently, so the
		 * priority can be set directly.
		 */
		*static_cast<typename Policy::Priority*>(currentTask) = *mutex;

		/*
		 * We know that the sleeper obtained here is actually a task,
		 * because only the block method can add elements to the
		 * waiter list, and that only accepts tasks.
		 *
		 * We also know that the task is not queued for a timeout,
		 * because locking with timeout is not supported (on purpose).
		 */
		if(Task* waken = static_cast<Task*>(mutex->waiters.popLowest())) {
			waken->blockedBy = nullptr;
			mutex->owner = waken;

			state.policy.addRunnable(waken);

			if(firstPreemptsSecond(waken, currentTask))
				switchToNext<true>();
		} else {
			mutex->owner = nullptr;
		}

	}

	return true;
}

#endif /* MUTEX_H_ */
