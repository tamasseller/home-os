/*
 * Helpers.h
 *
 *  Created on: 2017.05.26.
 *      Author: tooma
 */

#ifndef HELPERS_H_
#define HELPERS_H_

#include <stdint.h>

template<class Profile, template<class> class PolicyParam>
template<bool pendOld>
inline void Scheduler<Profile, PolicyParam>::
switchToNext()
{
	if(TaskBase* newTask = static_cast<TaskBase*>(static_cast<TaskBase*>(policy.popNext()))) {
		if(pendOld)
			policy.addRunnable(static_cast<TaskBase*>(Profile::Task::getCurrent()));

		newTask->switchTo();
	} else
		Profile::Task::suspendExecution();

}

/*
 * Helpers.
 */

template<class Profile, template<class> class PolicyParam>
template<class T>
inline uintptr_t Scheduler<Profile, PolicyParam>::
detypePtr(T* x) {
	return reinterpret_cast<uintptr_t>(x);
}

template<class Profile, template<class> class PolicyParam>
template<class T>
inline T* Scheduler<Profile, PolicyParam>::
entypePtr(uintptr_t  x) {
	return reinterpret_cast<T*>(x);
}

template<class Profile, template<class> class PolicyParam>
template<class RealEvent, class... Args>
inline void Scheduler<Profile, PolicyParam>::postEvent(RealEvent* event, Args... args) {
	eventList.issue(event, args...);
	Profile::CallGate::async(&Scheduler::doAsync);
}

#endif /* HELPERS_H_ */
