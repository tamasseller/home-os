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

#ifndef BASIC_H_
#define BASIC_H_

#include "Scheduler.h"

namespace home {

template<class... Args>
template<class Combiner>
inline void Scheduler<Args...>::submitEvent(Event* event, Combiner&& combiner) {
	state.eventList.issue(event, combiner);
}

template<class... Args>
template<class... T>
inline const char* Scheduler<Args...>::start(T... t) {
    /*
     * Initialize internal state.
     */
    Task* firstTask = state.policy.popNext();
    state.isRunning = true;

    /*
     * Initialize platform.
     */
    Profile::setTickHandler(&Scheduler<Args...>::onTick);
    Profile::setSyscallMapper(&syscallMapper);
    Profile::init(t...);

    /*
     * Start scheduling.
     */
    const char* ret = Profile::startFirst(firstTask);

    /*
     * Reset handlers to detect platform tear-down issues.
     */
    Profile::setTickHandler(nullptr);
    Profile::setSyscallMapper(nullptr);

    /*
     * Reset state to initial values.
     */
    state.~State();
    new(&state) State();

    return ret;
}

template<class... Args>
inline typename Scheduler<Args...>::Profile::TickType Scheduler<Args...>::getTick() {
    return Profile::getTick();
}

template<class... Args>
inline void Scheduler<Args...>::abort(const char* errorMessage) {
    syscall<SYSCALL(doAbort)>(reinterpret_cast<uintptr_t>(errorMessage));
}

template<class... Args>
inline uintptr_t Scheduler<Args...>::doAbort(uintptr_t errorMessage) {
    Profile::finishLast(reinterpret_cast<const char*>(errorMessage));
    return errorMessage;
}

}

#endif /* BASIC_H_ */
