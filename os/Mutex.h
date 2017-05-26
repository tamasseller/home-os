/*
 * Mutex.h
 *
 *  Created on: 2017.05.26.
 *      Author: tooma
 */

#ifndef MUTEX_H_
#define MUTEX_H_

#include "Helpers.h"

template<class Profile, class Policy>
class Scheduler<Profile, Policy>::Mutex: public MutexBase {
public:
	void init();
	void lock();
	void unlock();
};


template<class Profile, class Policy>
class Scheduler<Profile, Policy>::MutexBase {
	friend Scheduler;
	TaskBase *owner;
	pet::LinkedList<TaskBase> waiters;
	uint32_t counter;

	void init();
};


template<class Profile, class Policy>
void Scheduler<Profile, Policy>::Mutex::lock() {
	Profile::CallGate::sync(&Scheduler::doLock, detypePtr(static_cast<MutexBase*>(this)));
}

template<class Profile, class Policy>
void Scheduler<Profile, Policy>::Mutex::unlock() {
	Profile::CallGate::sync(&Scheduler::doUnlock, detypePtr(static_cast<MutexBase*>(this)));
}


template<class Profile, class Policy>
void Scheduler<Profile, Policy>::Mutex::init() {
	MutexBase::init();
}


template<class Profile, class Policy>
void Scheduler<Profile, Policy>::MutexBase::init()
{
	owner = nullptr;
}

template<class Profile, class Policy>
uintptr_t Scheduler<Profile, Policy>::doLock(uintptr_t mutexPtr)
{
	MutexBase* mutex = entypePtr<MutexBase>(mutexPtr);

	if(!mutex->owner) {
		mutex->owner = currentTask;
		mutex->counter = 0;
		// assert(mutex->waiters.empty();
	} else if(mutex->owner == currentTask) {
		mutex->counter++;
	} else {
		mutex->waiters.add(currentTask);
		bool ok = switchToNext();
		// assert(ok);
	}
}

template<class Profile, class Policy>
uintptr_t Scheduler<Profile, Policy>::doUnlock(uintptr_t mutexPtr)
{
	MutexBase* mutex = entypePtr<MutexBase>(mutexPtr);

	// assert(mutex->owner == currentTask);

	if(mutex->counter)
		mutex->counter--;
	else {
		auto it = mutex->waiters.iterator();
		while(it.current() && it.current()->next)
			it.step();

		if(TaskBase* waken = it.current()) {
			it.remove();
			mutex->owner = waken;
			policy.addRunnable(waken);
			if(!policy.canRunMore(currentTask)) {
				policy.addRunnable(currentTask);
				switchToNext();
			}
		} else {
			mutex->owner = nullptr;
		}
	}
}


#endif /* MUTEX_H_ */
