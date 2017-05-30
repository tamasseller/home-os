/*
 * WaitList.h
 *
 *  Created on: 2017.05.30.
 *      Author: tooma
 */

#ifndef WAITLIST_H_
#define WAITLIST_H_

#include "Scheduler.h"
#include "Waiter.h"
#include "Task.h"

#include "data/OrderedDoubleList.h"

template<class Profile, template<class> class PolicyParam>
class Scheduler<Profile, PolicyParam>::WaitList:
	private pet::OrderedDoubleList<typename Scheduler<Profile, PolicyParam>::Waiter> {
public:
	inline void add(TaskBase* elem);
	inline void remove(TaskBase* elem);
	inline TaskBase* peek();
	inline TaskBase* pop();
};

template<class Profile, template<class> class PolicyParam>
inline void Scheduler<Profile, PolicyParam>::WaitList::add(TaskBase* elem)
{
	this->pet::OrderedDoubleList<Waiter>::add(static_cast<Waiter*>(elem));
}

template<class Profile, template<class> class PolicyParam>
inline void Scheduler<Profile, PolicyParam>::WaitList::remove(TaskBase* elem)
{
	Waiter* waiter = static_cast<Waiter*>(elem);
	this->pet::OrderedDoubleList<Waiter>::remove(waiter);
	waiter->invalidate();
}

template<class Profile, template<class> class PolicyParam>
inline typename Scheduler<Profile, PolicyParam>::TaskBase* Scheduler<Profile, PolicyParam>::WaitList::peek()
{
	if(Waiter* ret = this->pet::OrderedDoubleList<Waiter>::lowest())
		return static_cast<TaskBase*>(ret);

	return nullptr;
}

template<class Profile, template<class> class PolicyParam>
inline typename Scheduler<Profile, PolicyParam>::TaskBase* Scheduler<Profile, PolicyParam>::WaitList::pop()
{
	if(Waiter* ret = this->pet::OrderedDoubleList<Waiter>::popLowest()) {
		ret->invalidate();
		return static_cast<TaskBase*>(ret);
	}

	return nullptr;
}

#endif /* WAITLIST_H_ */
