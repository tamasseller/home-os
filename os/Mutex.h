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
class Scheduler<Args...>::Mutex
{
	friend Scheduler<Args...>;
	static bool comparePriority(const Blockable&, const Blockable&);

	Task *owner;
	pet::OrderedDoubleList<Blockable, &Mutex::comparePriority> waiters;
	uintptr_t relockCounter;

public:
	void init();
	void lock();
	void unlock();
};

template<class... Args>
void Scheduler<Args...>::
Mutex::init() {
	owner = nullptr;
}

template<class... Args>
void Scheduler<Args...>::
Mutex::lock() {
	Profile::CallGate::sync(&Scheduler<Args...>::doLock, detypePtr(this));
}

template<class... Args>
void Scheduler<Args...>::
Mutex::unlock() {
	Profile::CallGate::sync(&Scheduler<Args...>::doUnlock, detypePtr(this));
}

template<class... Args>
bool Scheduler<Args...>::
Mutex::comparePriority(const Blockable& a, const Blockable& b) {
	return static_cast<const Task&>(a) < static_cast<const Task&>(b);
}

template<class... Args>
uintptr_t Scheduler<Args...>::
doLock(uintptr_t mutexPtr)
{
	Mutex* mutex = entypePtr<Mutex>(mutexPtr);
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
	Mutex* mutex = entypePtr<Mutex>(mutexPtr);
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
		if(Task* waken = static_cast<Task*>(mutex->waiters.popLowest())) {
			mutex->owner = waken;

			state.policy.addRunnable(waken);

			if(*currentTask < *waken)
				switchToNext<true>();
		} else {
			mutex->owner = nullptr;
		}
	}
}

#endif /* MUTEX_H_ */
