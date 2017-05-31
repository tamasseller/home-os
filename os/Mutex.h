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
class Scheduler<Args...>::
Mutex: public MutexBase {
public:
	void init();
	void lock();
	void unlock();
};

template<class... Args>
void Scheduler<Args...>::
Mutex::init() {
	MutexBase::init();
}

template<class... Args>
void Scheduler<Args...>::
Mutex::lock() {
	Profile::CallGate::sync(&Scheduler<Args...>::doLock, detypePtr(static_cast<MutexBase*>(this)));
}

template<class... Args>
void Scheduler<Args...>::
Mutex::unlock() {
	Profile::CallGate::sync(&Scheduler<Args...>::doUnlock, detypePtr(static_cast<MutexBase*>(this)));
}

/**
 * Internal mutex object.
 */
template<class... Args>
class Scheduler<Args...>::
MutexBase {
	friend Scheduler<Args...>;
	Task *owner;
	WaitList waiters;
	uintptr_t relockCounter;

	void init();
};

template<class... Args>
void Scheduler<Args...>::
MutexBase::init()
{
	owner = nullptr;
}

template<class... Args>
uintptr_t Scheduler<Args...>::
doLock(uintptr_t mutexPtr)
{
	MutexBase* mutex = entypePtr<MutexBase>(mutexPtr);
	Task* currentTask = static_cast<Task*>(Profile::Task::getCurrent());

	if(!mutex->owner) {
		mutex->owner = currentTask;
		mutex->relockCounter = 0;
		// assert(mutex->waiters.empty();
	} else if(mutex->owner == currentTask) {
		mutex->relockCounter++;
	} else {
		mutex->waiters.add(currentTask);
		switchToNext<false>();
	}
}

template<class... Args>
uintptr_t Scheduler<Args...>::
doUnlock(uintptr_t mutexPtr)
{
	MutexBase* mutex = entypePtr<MutexBase>(mutexPtr);
	Task* currentTask = static_cast<Task*>(Profile::Task::getCurrent());

	// assert(mutex->owner == currentTask);

	if(mutex->relockCounter)
		mutex->relockCounter--;
	else {
		/*
		 * We know that the sleeper obtained here is actually a task,
		 * because only the block method can add elements to the
		 * waiter list, and that only accepts tasks.
		 *
		 * We also know that the task is not queued for a timeout,
		 * because locking with timeout is not supported (on purpose).
		 */
		if(Task* waken = static_cast<Task*>(mutex->waiters.pop())) {
			mutex->owner = waken;

			state.policy.addRunnable(waken);

			if(*static_cast<Waiter*>(currentTask) < *waken)
				switchToNext<true>();
		} else {
			mutex->owner = nullptr;
		}
	}
}


#endif /* MUTEX_H_ */
