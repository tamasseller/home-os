/*
 * Event.h
 *
 *  Created on: 2017.05.28.
 *      Author: tooma
 */

#ifndef EVENT_H_
#define EVENT_H_

#include "Scheduler.h"

#include <stdint.h>

template<class... Args>
class Scheduler<Args...>::EventBase: public Scheduler<Args...>::AtomicList::Element {
	friend EventList;
	void (* const callback)(uintptr_t);
protected:
	inline EventBase(void (* const callback)(uintptr_t)): callback(callback) {}
};

template<class... Args>
template<class Child>
class Scheduler<Args...>::Event: public Scheduler<Args...>::EventBase {
public:
	inline Event(): EventBase(Child::execute) {}
};

template<class... Args>
class Scheduler<Args...>::EventList: public Scheduler<Args...>::AtomicList {
public:
	template<class Event, class... CombinerArgs>
	inline void issue(Event* event, CombinerArgs... args) {
		auto element = static_cast<typename AtomicList::Element*>(event);
		AtomicList::push(element, typename Event::Combiner(), args...);
	}

	inline void dispatch()
	{
		uintptr_t arg;
		while(typename AtomicList::Element* ret = AtomicList::pop(arg))
			static_cast<EventBase*>(ret)->callback(arg);
	}
};

#endif /* EVENT_H_ */
