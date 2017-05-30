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

template<class Profile, template<class> class PolicyParam>
class Scheduler<Profile, PolicyParam>::EventBase: public Scheduler<Profile, PolicyParam>::AtomicList::Element {
	friend EventList;
	void (* const callback)(uintptr_t);
protected:
	inline EventBase(void (* const callback)(uintptr_t)): callback(callback) {}
};

template<class Profile, template<class> class PolicyParam>
template<class Child>
class Scheduler<Profile, PolicyParam>::Event: public Scheduler<Profile, PolicyParam>::EventBase {
public:
	inline Event(): EventBase(Child::execute) {}
};

template<class Profile, template<class> class PolicyParam>
class Scheduler<Profile, PolicyParam>::EventList: public Scheduler<Profile, PolicyParam>::AtomicList {
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
