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
	inline Event(void (* const callback)(Event*, uintptr_t)): callback(callback) {}
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
		AtomicList::push(element, Combiner());
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
