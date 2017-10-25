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
		Task* currentTask = static_cast<Task*>(Profile::getCurrent());

		if(blocker->ActualBlocker::release(arg)) {
			if (Task *newTask = static_cast<Task*>(state.policy.peekNext())) {
				if (!currentTask || firstPreemptsSecond(newTask, currentTask))
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
