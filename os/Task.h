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

/**
 * Task front-end object.
 */
template<class Profile, template<class> class Policy>
template<class Child>
class Scheduler<Profile, Policy>::
Task: public TaskBase {
public:
	inline void start(void* stack, uint32_t stackSize);
};

template<class Profile, template<class> class Policy>
inline void Scheduler<Profile, Policy>::
yield() {
	Profile::CallGate::sync(&Scheduler::doYield);
}

template<class Profile, template<class> class Policy>
inline void Scheduler<Profile, Policy>::
sleep(uintptr_t time) {
	// assert(time <= INTPTR_MAX);
	Profile::CallGate::sync(&Scheduler::doSleep, time);
}


template<class Profile, template<class> class Policy>
inline void Scheduler<Profile, Policy>::
exit() {
	Profile::CallGate::sync(&Scheduler::doExit);
}

template<class Profile, template<class> class Policy>
template<class Child>
inline void Scheduler<Profile, Policy>::Task<Child>::
start(void* stack, uint32_t stackSize) {
	Profile::Task::template initialize<
		Child,
		&Child::run,
		&Scheduler::exit> (
				stack,
				stackSize,
				static_cast<Child*>(this));

	auto arg = detypePtr(static_cast<TaskBase*>(this));

	if(isRunning)
		Profile::CallGate::sync(&Scheduler<Profile, Policy>::doStartTask, arg);
	else
		doStartTask(arg);
}

/**
 * Internal task object.
 */
template<class Profile, template<class> class Policy>
class Scheduler<Profile, Policy>::
TaskBase: Profile::Task, Policy<TaskBase>::Task, Scheduler<Profile, Policy>::Sleeper {
	friend Scheduler;

	friend pet::DoubleList<TaskBase>;
	TaskBase *next, *prev;
};

template<class Profile, template<class> class Policy>
uintptr_t Scheduler<Profile, Policy>::
doStartTask(uintptr_t task) {
	policy.addRunnable(entypePtr<TaskBase>(task));
}

template<class Profile, template<class> class Policy>
uintptr_t Scheduler<Profile, Policy>::
doYield() {
	switchToNext<true>();
}

template<class Profile, template<class> class Policy>
uintptr_t Scheduler<Profile, Policy>::
doSleep(uintptr_t time) {
	TaskBase* currentTask = static_cast<TaskBase*>(Profile::Task::getCurrent());
	currentTask->deadline = Profile::Timer::getTick() + time;
	sleepList.add(currentTask);
	switchToNext<false>();
}

template<class Profile, template<class> class Policy>
uintptr_t Scheduler<Profile, Policy>::
doExit() {
	if(TaskBase* newTask = static_cast<TaskBase*>(policy.getNext())) {
		newTask->switchTo();
	} else {
		isRunning = false;
		Profile::Task::getCurrent()->finishLast();
	}
}

#endif /* TASK_H_ */
