/*
 * Helpers.h
 *
 *  Created on: 2017.05.26.
 *      Author: tooma
 */

#ifndef HELPERS_H_
#define HELPERS_H_

template<class Profile, template<class> class Policy>
template<bool pendOld>
inline void Scheduler<Profile, Policy>::
switchToNext()
{
	if(TaskBase* newTask = policy.getNext()) {
		if(pendOld)
			policy.addRunnable(static_cast<TaskBase*>(Profile::Task::getCurrent()));

		newTask->switchTo();
	} else
		Profile::Task::suspendExecution();

}

/*
 * Helpers.
 */

template<class Profile, template<class> class Policy>
template<class T>
inline uintptr_t Scheduler<Profile, Policy>::
detypePtr(T* x) {
	return reinterpret_cast<uintptr_t>(x);
}

template<class Profile, template<class> class Policy>
template<class T>
inline T* Scheduler<Profile, Policy>::
entypePtr(uintptr_t  x) {
	return reinterpret_cast<T*>(x);
}

template<class Profile, template<class> class Policy>
template<class RealEvent, class... Args>
inline void Scheduler<Profile, Policy>::postEvent(RealEvent* event, Args... args) {
	eventList.issue(event, args...);
	Profile::CallGate::async(&Scheduler::doAsync);
}

#endif /* HELPERS_H_ */
