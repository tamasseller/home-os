/*
 * Helpers.h
 *
 *  Created on: 2017.05.26.
 *      Author: tooma
 */

#ifndef HELPERS_H_
#define HELPERS_H_

#include "Scheduler.h"

template<class... Args>
template<bool pendOld, bool suspend>
inline void Scheduler<Args...>::switchToNext()
{
	if(Task* newTask = state.policy.popNext()) {
		if(pendOld) {
			if(Task* currentTask = static_cast<Task*>(Profile::getCurrent()))
					state.policy.addRunnable(currentTask);
		}

		Profile::switchToSync(newTask);
	} else if(suspend)
		Profile::suspendExecution();
}

template<class... Args>
template<class T>
inline uintptr_t Scheduler<Args...>::detypePtr(T* x)
{
	return reinterpret_cast<uintptr_t>(x);
}

template<class... Args>
template<class T>
inline T* Scheduler<Args...>::entypePtr(uintptr_t  x)
{
	return reinterpret_cast<T*>(x);
}

template<class... Args>
inline bool Scheduler<Args...>::firstPreemptsSecond(const Task* first, const Task *second)
{
	return *first < *second;
}

namespace {
	struct Helper {
		template<class Method, class... Args>
		static inline uintptr_t directCall(Method method, Args... args) {
			return method(args...);
		}
	};
}

template<class... Args>
template<class ... T>
inline uintptr_t Scheduler<Args...>::conditionalSyscall(T ... ops)
{
	if(state.isRunning)
		return Profile::sync(ops...);
	else
		return Helper::directCall(ops...);
}

/*
 * Obviously the assert function should not go through the active
 * branch when the system is intact, so LCOV_EXCL_START is placed
 * to avoid confusing the test coverage analysis.
 */

template<class... Args>
inline void Scheduler<Args...>::assert(bool cond, const char* msg) {
	if(assertEnabled && !cond) {
		Profile::fatalError(msg);
	}
}

/*
 * LCOV_EXCL_STOP is placed here to balance the previous lcov directive.
 */

#endif /* HELPERS_H_ */
