/*
 * Helpers.h
 *
 *  Created on: 2017.05.26.
 *      Author: tooma
 */

#ifndef HELPERS_H_
#define HELPERS_H_

template<class Profile, template<class> class Policy>
inline bool Scheduler<Profile, Policy>::
switchToNext()
{
	if(TaskBase* newTask = static_cast<TaskBase*>(policy.getNext())) {
		if(newTask != currentTask) {
			currentTask->switchTo(newTask);
			currentTask = newTask;
			return true;
		}
	}

	return false;
}

/*
 * Helpers.
 */

template<class Profile, template<class> class Policy>
template<class T>
uintptr_t Scheduler<Profile, Policy>::
detypePtr(T* x) {
	return reinterpret_cast<uintptr_t>(x);
}

template<class Profile, template<class> class Policy>
template<class T>
T* Scheduler<Profile, Policy>::
entypePtr(uintptr_t  x) {
	return reinterpret_cast<T*>(x);
}

#endif /* HELPERS_H_ */
