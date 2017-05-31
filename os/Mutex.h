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
	TaskBase *owner;
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
	TaskBase* currentTask = static_cast<TaskBase*>(Profile::Task::getCurrent());

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
	TaskBase* currentTask = static_cast<TaskBase*>(Profile::Task::getCurrent());

	// assert(mutex->owner == currentTask);

	if(mutex->relockCounter)
		mutex->relockCounter--;
	else {
		if(Waiter* waken = mutex->waiters.pop()) {
			mutex->owner = static_cast<TaskBase*>(waken);

			if(*static_cast<Waiter*>(currentTask) < *waken)
				switchToNext<true>();
			else
				state.policy.addRunnable(waken);

		} else {
			mutex->owner = nullptr;
		}
	}
}


#endif /* MUTEX_H_ */
