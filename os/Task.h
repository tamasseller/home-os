/*
 * Task.h
 *
 *  Created on: 2017.05.26.
 *      Author: tooma
 */

#ifndef TASK_H_
#define TASK_H_

#include "Scheduler.h"

/**
 * Task front-end object.
 */
template<class... Args>
class Scheduler<Args...>::Task: Policy::Priority, Profile::Task, Sleeper, Blockable
{
		template<class Child, void (Child::*entry)()>
		static void entryStub(void* self) {
			(static_cast<Child*>(self)->*entry)();
		}

		friend Scheduler<Args...>;
		friend PolicyBase;

		Blocker* blockedBy = nullptr;

		static Task* getTaskVirtual(Blockable* self) {
			return static_cast<Task*>(self);
		}

	public:
		template<class... StartArgs>
		inline void start(
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

		inline Task(): Blockable(&Task::getTaskVirtual) {}

	protected:

		template<class Child, void (Child::*entry)(), class... StartArgs>
		inline void start(void* stack, uintptr_t stackSize, StartArgs... startArgs) {
			start(&Task::entryStub<Child, entry>, stack, stackSize, static_cast<Child*>(this), startArgs...);
		}
};

template<class... Args>
inline void Scheduler<Args...>::yield() {
	Profile::CallGate::sync(&Scheduler<Args...>::doYield);
}

template<class... Args>
inline void Scheduler<Args...>::sleep(uintptr_t time)
{
	assert(time <= INTPTR_MAX, "Delay time too big!");
	Profile::CallGate::sync(&Scheduler<Args...>::doSleep, time);
}

template<class... Args>
inline void Scheduler<Args...>::exit()
{
	Profile::CallGate::sync(&Scheduler<Args...>::doExit);
} // LCOV_EXCL_LINE: this line is never reached.


template<class... Args>
uintptr_t Scheduler<Args...>::doStartTask(uintptr_t task)
{
	state.nTasks++;
	state.policy.addRunnable(entypePtr<Task>(task));

	return true;
}

template<class... Args>
uintptr_t Scheduler<Args...>::doYield()
{
	switchToNext<true, false>();
	return true;
}

template<class... Args>
uintptr_t Scheduler<Args...>::doSleep(uintptr_t time)
{
	Task* currentTask = static_cast<Task*>(Profile::Task::getCurrent());
	state.sleepList.delay(currentTask, time);
	switchToNext<false>();

	return true;
}

template<class... Args>
uintptr_t Scheduler<Args...>::doExit()
{
	if(--state.nTasks) {
		switchToNext<false>();
	} else {
		state.isRunning = false;
		Profile::Task::getCurrent()->finishLast();
	}

	return true;
}

#endif /* TASK_H_ */
