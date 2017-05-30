/*
 * Mutex.h
 *
 *  Created on: 2017.05.26.
 *      Author: tooma
 */

#ifndef MUTEX_H_
#define MUTEX_H_

#include "Helpers.h"

#include "data/DoubleList.h"

#include <stdint.h>

/**
 * Mutex front-end object.
 */
template<class Profile, template<class> class PolicyParam>
class Scheduler<Profile, PolicyParam>::
Mutex: public MutexBase {
public:
	void init();
	void lock();
	void unlock();
};

template<class Profile, template<class> class PolicyParam>
void Scheduler<Profile, PolicyParam>::
Mutex::init() {
	MutexBase::init();
}

template<class Profile, template<class> class PolicyParam>
void Scheduler<Profile, PolicyParam>::
Mutex::lock() {
	Profile::CallGate::sync(&Scheduler::doLock, detypePtr(static_cast<MutexBase*>(this)));
}

template<class Profile, template<class> class PolicyParam>
void Scheduler<Profile, PolicyParam>::
Mutex::unlock() {
	Profile::CallGate::sync(&Scheduler::doUnlock, detypePtr(static_cast<MutexBase*>(this)));
}

/**
 * Internal mutex object.
 */
template<class Profile, template<class> class PolicyParam>
class Scheduler<Profile, PolicyParam>::
MutexBase {
	friend Scheduler;
	TaskBase *owner;
	WaitList waiters;
	uintptr_t relockCounter;

	void init();
};

template<class Profile, template<class> class PolicyParam>
void Scheduler<Profile, PolicyParam>::
MutexBase::init()
{
	owner = nullptr;
}

template<class Profile, template<class> class PolicyParam>
uintptr_t Scheduler<Profile, PolicyParam>::
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

template<class Profile, template<class> class PolicyParam>
uintptr_t Scheduler<Profile, PolicyParam>::
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
				policy.addRunnable(waken);

		} else {
			mutex->owner = nullptr;
		}
	}
}


#endif /* MUTEX_H_ */
