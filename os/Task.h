/*
 * Task.h
 *
 *  Created on: 2017.05.26.
 *      Author: tooma
 */

#ifndef TASK_H_
#define TASK_H_

#include "Helpers.h"

/**
 * Task front-end object.
 */
template<class Profile, class Policy>
template<class Child>
class Scheduler<Profile, Policy>::Task: public TaskBase {
public:
	inline void start(void* stack, uint32_t stackSize);
};

template<class Profile, class Policy>
inline void Scheduler<Profile, Policy>::yield() {
	Profile::CallGate::sync(&Scheduler::doYield);
}

template<class Profile, class Policy>
inline void Scheduler<Profile, Policy>::exit() {
	Profile::CallGate::sync(&Scheduler::doExit);
}

template<class Profile, class Policy>
template<class Child>
inline void Scheduler<Profile, Policy>::Task<Child>::start(void* stack, uint32_t stackSize) {
	Profile::Task::template initialize<
		Child,
		&Child::run,
		&Scheduler::exit> (
				stack,
				stackSize,
				static_cast<Child*>(this));

	Profile::CallGate::sync(
			&Scheduler<Profile, Policy>::doStartTask,
			detypePtr(static_cast<TaskBase*>(this)));
}


/**
 * Task internal object.
 */
template<class Profile, class Policy>
class Scheduler<Profile, Policy>::TaskBase: Profile::Task, Policy::Task {
	friend Scheduler;
	friend pet::LinkedList<TaskBase>;

	TaskBase* next;
};

template<class Profile, class Policy>
uintptr_t Scheduler<Profile, Policy>::doStartTask(uintptr_t task) {
	policy.addRunnable(entypePtr<TaskBase>(task));
}

template<class Profile, class Policy>
uintptr_t Scheduler<Profile, Policy>::doYield() {
	policy.addRunnable(currentTask);
	switchToNext();
}

template<class Profile, class Policy>
uintptr_t Scheduler<Profile, Policy>::doExit() {
	if(!switchToNext())
		currentTask->finishLast();
}

#endif /* TASK_H_ */
