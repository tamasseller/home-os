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
	Policy::Priority,
	Profile::Task,
	Scheduler<Args...>::Sleeper,
	Scheduler<Args...>::Blockable
{
		template<class Child, void (Child::*entry)()> static void entryStub(void* self);

		friend Scheduler<Args...>;
		friend Policy;

		/*
		 * Used by waitables only!
		 */
		Blocker* waitsFor = nullptr;

		static Task* getTaskVirtual(Blockable*);
	protected:
		template<class Child, void (Child::*entry)(), class... StartArgs>
		inline void start(void* stack, uintptr_t stackSize, StartArgs...);

	public:
		inline Task();

		template<class... StartArgs>
		inline void start(void (*entry)(void*), void* stack, uintptr_t stackSize, void* arg, StartArgs...);
};

template<class... Args>
typename Scheduler<Args...>::Task* Scheduler<Args...>::Task::getTaskVirtual(Blockable* self) {
	return static_cast<Task*>(self);
}

template<class... Args>
inline Scheduler<Args...>::Task::Task():
	Blockable(&Task::getTaskVirtual) {}


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
template<class... StartArgs>
inline void Scheduler<Args...>::Task::start(
		void (*entry)(void*),
		void* stack,
		uintptr_t stackSize,
		void* arg,
		StartArgs... startArgs)
{
	Profile::Task::initialize(entry, &Scheduler<Args...>::exit, stack, stackSize, arg);
	Policy::initialize(this, startArgs...);

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
template<class Child, void (Child::*entry)(), class... StartArgs>
inline void Scheduler<Args...>::Task::start(void* stack, uintptr_t stackSize, StartArgs... startArgs)
{
	start(&Task::entryStub<Child, entry>, stack, stackSize, static_cast<Child*>(this), startArgs...);
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
	switchToNext<true, false>();
}

template<class... Args>
uintptr_t Scheduler<Args...>::
doSleep(uintptr_t time) {
	Task* currentTask = static_cast<Task*>(Profile::Task::getCurrent());
	state.sleepList.delay(currentTask, time);
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
