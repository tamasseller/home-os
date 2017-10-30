/*
 * AsyncBlocker.h
 *
 *  Created on: 2017.10.25.
 *      Author: tooma
 */

#ifndef ASYNCBLOCKER_H_
#define ASYNCBLOCKER_H_

#include "Scheduler.h"

template<class... Args>
template<class ActualBlocker>
class Scheduler<Args...>::AsyncBlocker: protected Event {
	static void doNotify(Event* event, uintptr_t arg) {
		ActualBlocker* blocker = static_cast<ActualBlocker*>(event);
		Scheduler<Args...>::doRelease<ActualBlocker>(reinterpret_cast<uintptr_t>(blocker), arg);
	}

protected:
	inline AsyncBlocker() :
			Event(&AsyncBlocker::doNotify) {
	}

public:
	inline void notifyFromInterrupt() {
		state.eventList.issue(this);
	}

    inline void notifyFromTask() {
        syscall(&Scheduler<Args...>::doRelease<ActualBlocker>, Registry<ActualBlocker>::getRegisteredId(static_cast<ActualBlocker*>(this)), (uintptr_t)1);
    }

};

#endif /* ASYNCBLOCKER_H_ */
