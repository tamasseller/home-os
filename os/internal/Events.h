/*
 * Events.h
 *
 *  Created on: 2017.05.28.
 *      Author: tooma
 */

#ifndef EVENTS_H_
#define EVENTS_H_

#include "Scheduler.h"

#include <stdint.h>

template<class... Args>
class Scheduler<Args...>::Event: Scheduler<Args...>::AtomicList::Element {
	friend EventList;
	void (* const callback)(Event*, uintptr_t);
protected:
	inline Event(void (* callback)(Event*, uintptr_t)): callback(callback) {}
};

template<class... Args>
class Scheduler<Args...>::EventList: Scheduler<Args...>::AtomicList {
	struct Combiner {
		inline bool operator()(uintptr_t old, uintptr_t& result) const {
			result = old+1;
			return true;
		}
	};
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

		Profile::CallGate::blockAsync();

		AtomicList::push(element, Combiner());

		Profile::CallGate::unblockAsync();
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

#endif /* EVENTS_H_ */
