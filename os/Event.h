/*
 * Event.h
 *
 *  Created on: 2017.05.28.
 *      Author: tooma
 */

#ifndef EVENT_H_
#define EVENT_H_

#include "Scheduler.h"
#include "AtomicList.h"

#include <stdint.h>

template<class Profile, template<class> class Policy>
class Scheduler<Profile, Policy>::EventBase: public Scheduler<Profile, Policy>::AtomicList::Element {
	friend EventList;
	void (* const callback)(uintptr_t);
protected:
	inline EventBase(void (* const callback)(uintptr_t)): callback(callback) {}
};

template<class Profile, template<class> class Policy>
template<class Child>
class Scheduler<Profile, Policy>::Event: public Scheduler<Profile, Policy>::EventBase {
public:
	inline Event(): EventBase(Child::execute) {}
};

template<class Profile, template<class> class Policy>
class Scheduler<Profile, Policy>::EventList: public Scheduler<Profile, Policy>::AtomicList {
public:
	template<class Event, class... Args>
	inline void issue(Event* event, Args... args) {
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
