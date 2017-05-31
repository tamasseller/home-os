/*
 * SleepList.h
 *
 *  Created on: 2017.05.28.
 *      Author: tooma
 */

#ifndef SLEEPLIST_H_
#define SLEEPLIST_H_

#include "data/OrderedDoubleList.h"

template<class... Args>
class Scheduler<Args...>::SleepList:
	private pet::OrderedDoubleList<typename Scheduler<Args...>::Sleeper> {
public:
	inline void add(TaskBase* elem);
	inline void remove(TaskBase* elem);
	inline TaskBase* peek();
	inline TaskBase* pop();
};

template<class... Args>
inline void Scheduler<Args...>::SleepList:: add(TaskBase* elem)
{
	this->pet::OrderedDoubleList<Sleeper>::add(static_cast<Sleeper*>(elem));
}

template<class... Args>
inline void Scheduler<Args...>::SleepList:: remove(TaskBase* elem)
{
	Sleeper* sleeper = static_cast<Sleeper*>(elem);
	this->pet::OrderedDoubleList<Sleeper>::remove();
	sleeper->invalidate();
}

template<class... Args>
inline typename Scheduler<Args...>::TaskBase* Scheduler<Args...>::SleepList::peek()
{
	if(Sleeper* ret = this->pet::OrderedDoubleList<Sleeper>::lowest())
		return static_cast<TaskBase*>(ret);

	return nullptr;
}

template<class... Args>
inline typename Scheduler<Args...>::TaskBase* Scheduler<Args...>::SleepList::pop()
{
	if(Sleeper* ret = this->pet::OrderedDoubleList<Sleeper>::popLowest()) {
		ret->invalidate();
		return static_cast<TaskBase*>(ret);
	}

	return nullptr;
}

#endif /* SLEEPLIST_H_ */
