/*
 * Task.h
 *
 *  Created on: 2017.05.26.
 *      Author: tooma
 */

#ifndef TASK_H_
#define TASK_H_

#include "Scheduler.h"

#include <stdint.h>

/**
 * Task front-end object.
 */
template<class... Args>
template<class Child>
class Scheduler<Args...>::
Task: public TaskBase {
public:
	inline void start(void* stack, uintptr_t stackSize);
};

template<class... Args>
inline void Scheduler<Args...>::
yield() {
	Profile::CallGate::sync(&Scheduler<Args...>::doYield);
}

template<class... Args>
inline void Scheduler<Args...>::
sleep(uintptr_t time) {
	// assert(time <= INTPTR_MAX);
	Profile::CallGate::sync(&Scheduler<Args...>::doSleep, time);
}

template<class... Args>
inline void Scheduler<Args...>::
exit() {
	Profile::CallGate::sync(&Scheduler<Args...>::doExit);
}

template<class... Args>
template<class Child>
inline void Scheduler<Args...>::Task<Child>::
start(void* stack, uintptr_t stackSize) {
	Profile::Task::template initialize<
		Child,
		&Child::run,
		&Scheduler<Args...>::exit> (
				stack,
				stackSize,
				static_cast<Child*>(this));

	auto arg = detypePtr(static_cast<TaskBase*>(this));

	if(state.isRunning)
		Profile::CallGate::sync(&Scheduler<Args...>::doStartTask, arg);
	else
		doStartTask(arg);
}

/**
 * Internal task object.
 */
template<class... Args>
class Scheduler<Args...>::TaskBase:
	Profile::Task,
	Scheduler<Args...>::Sleeper,
	Scheduler<Args...>::Waiter {
	friend Scheduler<Args...>;
	WaitList* waitList;
};

template<class... Args>
uintptr_t Scheduler<Args...>::
doStartTask(uintptr_t task) {
	state.nTasks++;
	state.policy.addRunnable(entypePtr<TaskBase>(task));
}

template<class... Args>
uintptr_t Scheduler<Args...>::
doYield() {
	switchToNext<true>();
}

template<class... Args>
uintptr_t Scheduler<Args...>::
doSleep(uintptr_t time) {
	TaskBase* currentTask = static_cast<TaskBase*>(Profile::Task::getCurrent());
	currentTask->deadline = Profile::Timer::getTick() + time;
	state.sleepList.add(currentTask);
	switchToNext<false>();
}

template<class... Args>
uintptr_t Scheduler<Args...>::
doExit() {
	if(--state.nTasks) {
		switchToNext<false>();
	} else {
		state.isRunning = false;
		Profile::Task::getCurrent()->finishLast();
	}
}

#endif /* TASK_H_ */
