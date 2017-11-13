/*******************************************************************************
 *
 * Copyright (c) 2017 Seller Tamás. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *******************************************************************************/

#ifndef ASYNCBLOCKER_H_
#define ASYNCBLOCKER_H_

#include "Scheduler.h"

namespace home {

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
        Registry<ActualBlocker>::check(static_cast<ActualBlocker*>(this));
		state.eventList.issue(this);
	}

    inline void notifyFromTask() {
        syscall<SYSCALL(doRelease<ActualBlocker>)>(Registry<ActualBlocker>::getRegisteredId(static_cast<ActualBlocker*>(this)), (uintptr_t)1);
    }

};

}

#endif /* ASYNCBLOCKER_H_ */
