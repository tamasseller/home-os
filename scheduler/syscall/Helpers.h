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
inline typename Scheduler<Args...>::Task* Scheduler<Args...>::getCurrentTask() {
	if(auto platformTask = Profile::getCurrent())
		return static_cast<Task*>(platformTask);

	return nullptr;
}

namespace detail {
	template<class T>
	class ObjectInitializer: T {
		constexpr static inline void* operator new(size_t, ObjectInitializer* r) { return r; }
	public:
		static inline void reset(T* object) {
			ObjectInitializer* wrapped = reinterpret_cast<ObjectInitializer*>(object);
			wrapped->~ObjectInitializer<T>();
			new(wrapped) ObjectInitializer;
		}
	};
}

template<class... Args>
template<class T>
inline void Scheduler<Args...>::resetObject(T* object) {
	detail::ObjectInitializer<T>::reset(object);
}


template<class... Args>
template<bool pendOld, bool suspend>
inline void Scheduler<Args...>::switchToNext()
{
	if(Task* newTask = state.policy.popNext()) {
		if(pendOld) {
			if(Task* currentTask = getCurrentTask())
					state.policy.addRunnable(currentTask);
		}

		Profile::switchToSync(newTask);
	} else if(suspend)
		Profile::suspendExecution();
}

/*
 * Obviously the assert function should not go through the active
 * branch when the system is intact, so LCOV_EXCL_START is placed
 * to avoid confusing the test coverage analysis.
 */

template<class... Args>
template<class Dummy> struct Scheduler<Args...>::AssertSwitch<true, Dummy> {
	static inline void assert(bool cond, const char* msg) {
		// TODO determine context somehow and call abort from task.
		if(!cond)
			Scheduler<Args...>::Profile::finishLast(msg);
	}
};

template<class... Args>
template<class Dummy> struct Scheduler<Args...>::AssertSwitch<false, Dummy> {
	static inline void assert(bool cond, const char* msg) {}
};

template<class... Args>
inline void Scheduler<Args...>::assert(bool cond, const char* msg)
{
	AssertSwitch<assertEnabled>::assert(cond, msg);
}

/*
 * LCOV_EXCL_STOP is placed here to balance the previous lcov directive.
 */

#endif /* HELPERS_H_ */
