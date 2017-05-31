/*
 * Helpers.h
 *
 *  Created on: 2017.05.26.
 *      Author: tooma
 */

#ifndef HELPERS_H_
#define HELPERS_H_

#include <stdint.h>

template<class... Args>
template<bool pendOld>
inline void Scheduler<Args...>::
switchToNext()
{
	if(TaskBase* newTask = static_cast<TaskBase*>(static_cast<TaskBase*>(state.policy.popNext()))) {
		if(pendOld)
			state.policy.addRunnable(static_cast<TaskBase*>(Profile::Task::getCurrent()));

		newTask->switchTo();
	} else
		Profile::Task::suspendExecution();

}

/*
 * Helpers.
 */

template<class... Args>
template<class T>
inline uintptr_t Scheduler<Args...>::
detypePtr(T* x) {
	return reinterpret_cast<uintptr_t>(x);
}

template<class... Args>
template<class T>
inline T* Scheduler<Args...>::
entypePtr(uintptr_t  x) {
	return reinterpret_cast<T*>(x);
}

template<class... Args>
template<class RealEvent, class... CombinerArgs>
inline void Scheduler<Args...>::postEvent(RealEvent* event, CombinerArgs... args) {
	state.eventList.issue(event, args...);
	Profile::CallGate::async(&Scheduler<Args...>::doAsync);
}

#endif /* HELPERS_H_ */
