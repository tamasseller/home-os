/*
 * WaitList.h
 *
 *  Created on: 2017.05.30.
 *      Author: tooma
 */

#ifndef WAITLIST_H_
#define WAITLIST_H_

#include "data/OrderedDoubleList.h"

template<class... Args>
class Scheduler<Args...>::WaitList:
	private pet::OrderedDoubleList<typename Scheduler<Args...>::Waiter> {
public:
	inline void add(TaskBase* elem);
	inline void remove(TaskBase* elem);
	inline TaskBase* peek();
	inline TaskBase* pop();
};

template<class... Args>
inline void Scheduler<Args...>::WaitList::add(TaskBase* elem)
{
	this->pet::OrderedDoubleList<Waiter>::add(static_cast<Waiter*>(elem));
}

template<class... Args>
inline void Scheduler<Args...>::WaitList::remove(TaskBase* elem)
{
	Waiter* waiter = static_cast<Waiter*>(elem);
	this->pet::OrderedDoubleList<Waiter>::remove(waiter);
	waiter->invalidate();
}

template<class... Args>
inline typename Scheduler<Args...>::TaskBase* Scheduler<Args...>::WaitList::peek()
{
	if(Waiter* ret = this->pet::OrderedDoubleList<Waiter>::lowest())
		return static_cast<TaskBase*>(ret);

	return nullptr;
}

template<class... Args>
inline typename Scheduler<Args...>::TaskBase* Scheduler<Args...>::WaitList::pop()
{
	if(Waiter* ret = this->pet::OrderedDoubleList<Waiter>::popLowest()) {
		ret->invalidate();
		return static_cast<TaskBase*>(ret);
	}

	return nullptr;
}

#endif /* WAITLIST_H_ */
