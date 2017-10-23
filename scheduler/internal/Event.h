/*
 * Event.h
 *
 *  Created on: 2017.06.11.
 *      Author: tooma
 */

#ifndef EVENT_H_
#define EVENT_H_

template<class... Args>
class Scheduler<Args...>::Event: AtomicList::Element
{
	friend EventList;
	void (* const callback)(Event*, uintptr_t);
protected:
	inline Event(void (* callback)(Event*, uintptr_t)): callback(callback) {}
};

#endif /* EVENT_H_ */
