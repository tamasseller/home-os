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
class Scheduler<Args...>::Task:
	Profile::Task,
	Scheduler<Args...>::Sleeper,
	Scheduler<Args...>::Blockable
{
		template<class Child, void (Child::*entry)()> static void entryStub(void* self);

		friend Scheduler<Args...>;

		Waitable* waitsFor = nullptr;
	protected:
		template<class Child, void (Child::*entry)()>
		inline void start(void* stack, uintptr_t stackSize);

	public:
		inline void start(void (*entry)(void*), void* stack, uintptr_t stackSize, void* arg);
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
inline void Scheduler<Args...>::Task::start(
		void (*entry)(void*),
		void* stack,
		uintptr_t stackSize,
		void* arg)
{
	Profile::Task::initialize(entry, &Scheduler<Args...>::exit, stack, stackSize, arg);

	auto task = detypePtr(static_cast<Task*>(this));

	if(state.isRunning)
		Profile::CallGate::sync(&Scheduler<Args...>::doStartTask, task);
	else
		doStartTask(task);
}

template<class... Args>
template<class Child, void (Child::*entry)()>
void Scheduler<Args...>::Task::entryStub(void* self) {
	(static_cast<Child*>(self)->*entry)();
}

template<class... Args>
template<class Child, void (Child::*entry)()>
inline void Scheduler<Args...>::Task::start(void* stack, uintptr_t stackSize)
{
	start(&Task::entryStub<Child, entry>, stack, stackSize, static_cast<Child*>(this));
}


template<class... Args>
uintptr_t Scheduler<Args...>::
doStartTask(uintptr_t task) {
	state.nTasks++;
	state.policy.addRunnable(entypePtr<Task>(task));
}

template<class... Args>
uintptr_t Scheduler<Args...>::
doYield() {
	switchToNext<true>();
}

template<class... Args>
uintptr_t Scheduler<Args...>::
doSleep(uintptr_t time) {
	Task* currentTask = static_cast<Task*>(Profile::Task::getCurrent());
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
