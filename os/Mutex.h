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
template<class Profile, template<class> class Policy>
class Scheduler<Profile, Policy>::
Mutex: public MutexBase {
public:
	void init();
	void lock();
	void unlock();
};

template<class Profile, template<class> class Policy>
void Scheduler<Profile, Policy>::
Mutex::init() {
	MutexBase::init();
}

template<class Profile, template<class> class Policy>
void Scheduler<Profile, Policy>::
Mutex::lock() {
	Profile::CallGate::sync(&Scheduler::doLock, detypePtr(static_cast<MutexBase*>(this)));
}

template<class Profile, template<class> class Policy>
void Scheduler<Profile, Policy>::
Mutex::unlock() {
	Profile::CallGate::sync(&Scheduler::doUnlock, detypePtr(static_cast<MutexBase*>(this)));
}

/**
 * Internal mutex object.
 */
template<class Profile, template<class> class Policy>
class Scheduler<Profile, Policy>::
MutexBase {
	friend Scheduler;
	TaskBase *owner;
	pet::DoubleList<TaskBase> waiters;
	uintptr_t relockCounter;

	void init();
};

template<class Profile, template<class> class Policy>
void Scheduler<Profile, Policy>::
MutexBase::init()
{
	owner = nullptr;
}

template<class Profile, template<class> class Policy>
uintptr_t Scheduler<Profile, Policy>::
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
		mutex->waiters.fastAddBack(currentTask);
		switchToNext<false>();
	}
}

template<class Profile, template<class> class Policy>
uintptr_t Scheduler<Profile, Policy>::
doUnlock(uintptr_t mutexPtr)
{
	MutexBase* mutex = entypePtr<MutexBase>(mutexPtr);
	TaskBase* currentTask = static_cast<TaskBase*>(Profile::Task::getCurrent());

	// assert(mutex->owner == currentTask);

	if(mutex->relockCounter)
		mutex->relockCounter--;
	else {
		if(TaskBase* waken = mutex->waiters.popFront()) {
			mutex->owner = waken;

			if(policy.isHigherPriority(waken, currentTask))
				switchToNext<true>();
			else
				policy.addRunnable(waken);

		} else {
			mutex->owner = nullptr;
		}
	}
}


#endif /* MUTEX_H_ */
