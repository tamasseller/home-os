/*
 * Task.h
 *
 *  Created on: 2017.05.26.
 *      Author: tooma
 */

#ifndef TASK_H_
#define TASK_H_

#include "Helpers.h"
#include "Sleeper.h"

#include <stdint.h>

/**
 * Task front-end object.
 */
template<class Profile, template<class> class PolicyParam>
template<class Child>
class Scheduler<Profile, PolicyParam>::
Task: public TaskBase {
public:
	inline void start(void* stack, uintptr_t stackSize);
};

template<class Profile, template<class> class PolicyParam>
inline void Scheduler<Profile, PolicyParam>::
yield() {
	Profile::CallGate::sync(&Scheduler::doYield);
}

template<class Profile, template<class> class PolicyParam>
inline void Scheduler<Profile, PolicyParam>::
sleep(uintptr_t time) {
	// assert(time <= INTPTR_MAX);
	Profile::CallGate::sync(&Scheduler::doSleep, time);
}


template<class Profile, template<class> class PolicyParam>
inline void Scheduler<Profile, PolicyParam>::
exit() {
	Profile::CallGate::sync(&Scheduler::doExit);
}

template<class Profile, template<class> class PolicyParam>
template<class Child>
inline void Scheduler<Profile, PolicyParam>::Task<Child>::
start(void* stack, uintptr_t stackSize) {
	Profile::Task::template initialize<
		Child,
		&Child::run,
		&Scheduler::exit> (
				stack,
				stackSize,
				static_cast<Child*>(this));

	auto arg = detypePtr(static_cast<TaskBase*>(this));

	if(isRunning)
		Profile::CallGate::sync(&Scheduler<Profile, PolicyParam>::doStartTask, arg);
	else
		doStartTask(arg);
}

/**
 * Internal task object.
 */
template<class Profile, template<class> class PolicyParam>
class Scheduler<Profile, PolicyParam>::TaskBase:
	Profile::Task,
	Scheduler<Profile, PolicyParam>::Sleeper,
	Scheduler<Profile, PolicyParam>::Waiter {
	friend Scheduler;
	WaitList* waitList;
};

template<class Profile, template<class> class PolicyParam>
uintptr_t Scheduler<Profile, PolicyParam>::
doStartTask(uintptr_t task) {
	nTasks++;
	policy.addRunnable(entypePtr<TaskBase>(task));
}

template<class Profile, template<class> class PolicyParam>
uintptr_t Scheduler<Profile, PolicyParam>::
doYield() {
	switchToNext<true>();
}

template<class Profile, template<class> class PolicyParam>
uintptr_t Scheduler<Profile, PolicyParam>::
doSleep(uintptr_t time) {
	TaskBase* currentTask = static_cast<TaskBase*>(Profile::Task::getCurrent());
	currentTask->deadline = Profile::Timer::getTick() + time;
	sleepList.add(currentTask);
	switchToNext<false>();
}

template<class Profile, template<class> class PolicyParam>
uintptr_t Scheduler<Profile, PolicyParam>::
doExit() {
	if(--nTasks) {
		switchToNext<false>();
	} else {
		isRunning = false;
		Profile::Task::getCurrent()->finishLast();
	}
}

#endif /* TASK_H_ */
