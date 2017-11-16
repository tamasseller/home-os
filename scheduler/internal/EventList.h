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

#ifndef EVENTLIST_H_
#define EVENTLIST_H_

#include "Scheduler.h"

namespace home {

template<class... Args>
class Scheduler<Args...>::EventList: SharedAtomicList
{
	struct DefaultCombiner {
		inline bool operator()(uintptr_t old, uintptr_t& result) const {
			result = old+1;
			return true;
		}
	};

	static void dispatchTrampoline()
	{
		state.eventList.dispatch();
	}

	inline void dispatch()
	{
		uintptr_t arg;
		while(true) {
			auto reader = SharedAtomicList::read();

			if(auto* element = reader.pop(arg)) {
				do {
					Event* event = static_cast<Event*>(element);
					event->callback(event, arg);
				} while((element = reader.pop(arg)) != nullptr);
			} else
				break;
		}
	}

	Atomic<uintptr_t> criticality;

public:
	template<class Combiner = DefaultCombiner>
	inline void issue(Event* event, Combiner&& combiner = Combiner()) {
		/*
		 * The scheduling needs to be blocked while the event is added,
		 * because if the task gets preempted between the argument setting
		 * and list insertion step, then the _event_ instance could get
		 * stuck, meaning that it is only delivered once the task that
		 * started the insertion (and got preempted during it) gets to
		 * finish the operation.
		 *
		 * If this was left without protection then it could cause a
		 * nasty situations when the task that got the event stuck is
		 * a low priority one, and higher priority tasks can delay it
		 * indefinitely.
		 */

		criticality.increment();

		SharedAtomicList::push(event, combiner);

		uintptr_t result = criticality.decrement();

		/*
		 * The race condition here, introduced by using the value of the previous
		 * atomic operation non-atomically is known to be benign, the worst that
		 * can happen is that the asynchronous call is issued spuriously, but only
		 * outside the critical section, this is ensured by the atomicity of the
		 * counter accesses.
		 */

		if(result == 1)
			Profile::async(&EventList::dispatchTrampoline);
	}
};

}

#endif /* EVENTLIST_H_ */
