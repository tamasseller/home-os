/*
 * Mutex.h
 *
 *  Created on: 2017.05.26.
 *      Author: tooma
 */

#ifndef MUTEX_H_
#define MUTEX_H_

#include "Scheduler.h"

#include <stdint.h>

/**
 * Mutex front-end object.
 */
template<class... Args>
class Scheduler<Args...>::Mutex: Policy::Priority
{
	friend Scheduler<Args...>;

	static bool comparePriority(const Blockable& a, const Blockable& b) {
		return firstPreemptsSecond(static_cast<const Task*>(&a), static_cast<const Task*>(&b));
	}

	Task *owner;
	pet::OrderedDoubleList<Blockable, &Mutex::comparePriority> waiters;
	uintptr_t relockCounter;

public:
	void init() {
		owner = nullptr;
	}

	void lock() {
		Profile::CallGate::sync(&Scheduler<Args...>::doLock, detypePtr(this));
	}

	void unlock() {
		Profile::CallGate::sync(&Scheduler<Args...>::doUnlock, detypePtr(this));
	}
};

template<class... Args>
uintptr_t Scheduler<Args...>::
doLock(uintptr_t mutexPtr)
{
	Mutex* mutex = entypePtr<Mutex>(mutexPtr);
	Task* currentTask = static_cast<Task*>(Profile::Task::getCurrent());

	// TODO set waitsFor

	if(!mutex->owner) {
		mutex->owner = currentTask;
		mutex->relockCounter = 0;
		// assert(mutex->waiters.empty());

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
		 *
		 * TODO use waitsFor instead of the policy (it can be blocked by another mutex or waitable too).
		 */
		if(firstPreemptsSecond(currentTask, mutex->owner)) {
			state.policy.inheritPriority(mutex->owner, currentTask);
		}

		mutex->waiters.add(currentTask);
		switchToNext<false>();
	}
}

template<class... Args>
uintptr_t Scheduler<Args...>::
doUnlock(uintptr_t mutexPtr)
{
	Mutex* mutex = entypePtr<Mutex>(mutexPtr);
	Task* currentTask = static_cast<Task*>(Profile::Task::getCurrent());

	// assert(mutex->owner == currentTask);

	if(mutex->relockCounter)
		mutex->relockCounter--;
	else {
		/*
		 * Restore priority. Here the _currentTask_ is executing, so it
		 * is sure to not be handled by the policy currently, so the
		 * priority can be set directly.
		 */
		*static_cast<typename Policy::Priority*>(currentTask) = *mutex;

		// TODO reset waitsFor

		/*
		 * We know that the sleeper obtained here is actually a task,
		 * because only the block method can add elements to the
		 * waiter list, and that only accepts tasks.
		 *
		 * We also know that the task is not queued for a timeout,
		 * because locking with timeout is not supported (on purpose).
		 */
		if(Task* waken = static_cast<Task*>(mutex->waiters.popLowest())) {
			mutex->owner = waken;

			state.policy.addRunnable(waken);

			if(firstPreemptsSecond(waken, currentTask))
				switchToNext<true>();
		} else {
			mutex->owner = nullptr;
		}

	}
}

#endif /* MUTEX_H_ */
