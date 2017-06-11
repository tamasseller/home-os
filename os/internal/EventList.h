/*
 * EventList.h
 *
 *  Created on: 2017.05.28.
 *      Author: tooma
 */

#ifndef EVENTLIST_H_
#define EVENTLIST_H_

#include "Scheduler.h"

template<class... Args>
class Scheduler<Args...>::EventList: AtomicList
{
	struct Combiner {
		inline bool operator()(uintptr_t old, uintptr_t& result) const {
			result = old+1;
			return true;
		}
	};

	static void dispatchTrampoline()
	{
		state.eventList.dispatch();
	}

	Atomic<uintptr_t> criticality;

public:
	inline void issue(Event* event) {
		auto element = static_cast<typename AtomicList::Element*>(event);

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

		AtomicList::push(element, Combiner());

		uintptr_t result = criticality.decrement();

		/*
		 * The race condition here, introduced by using the value of the previous
		 * atomic operation non-atomically is known to be benign, the worst that
		 * can happen is that the asynchronous call is issued spuriously, but only
		 * outside the critical section, this is ensured by the atomicity of the
		 * counter accesses.
		 */

		if(result == 1)
			Profile::CallGate::async(&EventList::dispatchTrampoline);
	}

	inline void dispatch()
	{
		uintptr_t arg;
		for(auto reader = AtomicList::read(); auto* element = reader.pop(arg);) {
			Event* event = static_cast<Event*>(element);
			event->callback(event, arg);
		}
	}
};

#endif /* EVENTLIST_H_ */
