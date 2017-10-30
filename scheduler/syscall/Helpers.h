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
inline void Scheduler<Args...>::assert(bool cond, const char* msg) {
	if(assertEnabled && !cond) {
		Profile::fatalError(msg);
	}
}

/*
 * LCOV_EXCL_STOP is placed here to balance the previous lcov directive.
 */

#endif /* HELPERS_H_ */
