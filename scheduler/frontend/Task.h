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

		typename Policy::Priority &accessPriority() {
			return *static_cast<typename Policy::Priority*>(this);
		}

		typename Policy::Priority getPriority() const {
			return *static_cast<const typename Policy::Priority*>(this);
		}

		static void wakeVirtual(Sleeper* sleeper)
		{
			Task* self = static_cast<Task*>(sleeper);

			if(self->blockedBy) {
				self->blockedBy->remove(self, nullptr);
				self->blockedBy = nullptr;
			}

			state.policy.addRunnable(self);
		}

		void* operator new(size_t, void* r) {return r;}

	public:
		template<class... StartArgs>
		inline void start(
				void (*entry)(void*),
				void* stack,
				uintptr_t stackSize,
				void* arg,
				StartArgs... startArgs)
		{
			resetObject(this);

			Profile::initialize(this, entry, &Scheduler<Args...>::exit, stack, stackSize, arg);
			Policy::initialize(this, startArgs...);

			auto task = reinterpret_cast<uintptr_t>(static_cast<Task*>(this));

			conditionalSyscall<SYSCALL(doStartTask)>(task);
		}

		inline Task(): Sleeper(&Task::wakeVirtual), Blockable(&Task::getTaskVirtual) {}

	protected:

		template<class Child, void (Child::*entry)(), class... StartArgs>
		inline void start(void* stack, uintptr_t stackSize, StartArgs... startArgs) {
			start(&Task::entryStub<Child, entry>, stack, stackSize, static_cast<Child*>(this), startArgs...);
		}
};

template<class... Args>
inline void Scheduler<Args...>::yield() {
    syscall<SYSCALL(doYield)>();
}

template<class... Args>
inline void Scheduler<Args...>::sleep(uintptr_t time)
{
	syscall<SYSCALL(doSleep)>(time);
}

template<class... Args>
inline void Scheduler<Args...>::exit()
{
    syscall<SYSCALL(doExit)>();
} // LCOV_EXCL_LINE: this line is never reached.


template<class... Args>
uintptr_t Scheduler<Args...>::doStartTask(uintptr_t taskPtr)
{
	state.nTasks++;
	Task* task = reinterpret_cast<Task*>(taskPtr);
	state.policy.addRunnable(task);

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
	assert(time <= INTPTR_MAX, ErrorStrings::taskDelayTooBig);
	state.sleepList.delay(getCurrentTask(), time);
	switchToNext<false>();

	return true;
}

template<class... Args>
uintptr_t Scheduler<Args...>::doExit()
{
	if(--state.nTasks)
		switchToNext<false>();
	else
		doAbort(0);

	return 0;
}

#endif /* TASK_H_ */
