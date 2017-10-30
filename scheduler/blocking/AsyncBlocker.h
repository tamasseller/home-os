/*
 * AsyncBlocker.h
 *
 *  Created on: 2017.10.25.
 *      Author: tooma
 */

#ifndef ASYNCBLOCKER_H_
#define ASYNCBLOCKER_H_

#include "Scheduler.h"

template<class ... Args>
template<class ActualBlocker>
class Scheduler<Args...>::AsyncBlocker: protected Event {
	static void doNotify(Event* event, uintptr_t arg) {
		ActualBlocker* blocker = static_cast<ActualBlocker*>(event);
		Registry<ActualBlocker>::check(blocker);
		Task* currentTask = getCurrentTask();

		if(blocker->ActualBlocker::release(arg)) {
			if (Task *newTask = state.policy.peekNext()) {
				if (!currentTask || (newTask->getPriority() < currentTask->getPriority()))
					switchToNext<true>();
			}
		}
	}

protected:
	inline AsyncBlocker() :
			Event(&AsyncBlocker::doNotify) {
	}

public:
	inline void notify() {
		state.eventList.issue(this);
	}

};

#endif /* ASYNCBLOCKER_H_ */
