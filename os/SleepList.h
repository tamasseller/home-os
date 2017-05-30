/*
 * SleepList.h
 *
 *  Created on: 2017.05.28.
 *      Author: tooma
 */

#ifndef SLEEPLIST_H_
#define SLEEPLIST_H_

#include "Scheduler.h"
#include "Sleeper.h"
#include "Task.h"

#include "data/OrderedDoubleList.h"

template<class Profile, template<class> class PolicyParam>
class Scheduler<Profile, PolicyParam>::SleepList:
	private pet::OrderedDoubleList<typename Scheduler<Profile, PolicyParam>::Sleeper> {
public:
	inline void add(TaskBase* elem);
	inline void remove(TaskBase* elem);
	inline TaskBase* peek();
	inline TaskBase* pop();
};

template<class Profile, template<class> class PolicyParam>
inline void Scheduler<Profile, PolicyParam>::SleepList:: add(TaskBase* elem)
{
	this->pet::OrderedDoubleList<Sleeper>::add(static_cast<Sleeper*>(elem));
}

template<class Profile, template<class> class PolicyParam>
inline void Scheduler<Profile, PolicyParam>::SleepList:: remove(TaskBase* elem)
{
	Sleeper* sleeper = static_cast<Sleeper*>(elem);
	this->pet::OrderedDoubleList<Sleeper>::remove();
	sleeper->invalidate();
}

template<class Profile, template<class> class PolicyParam>
inline typename Scheduler<Profile, PolicyParam>::TaskBase* Scheduler<Profile, PolicyParam>::SleepList::peek()
{
	if(Sleeper* ret = this->pet::OrderedDoubleList<Sleeper>::lowest())
		return static_cast<TaskBase*>(ret);

	return nullptr;
}

template<class Profile, template<class> class PolicyParam>
inline typename Scheduler<Profile, PolicyParam>::TaskBase* Scheduler<Profile, PolicyParam>::SleepList::pop()
{
	if(Sleeper* ret = this->pet::OrderedDoubleList<Sleeper>::popLowest()) {
		ret->invalidate();
		return static_cast<TaskBase*>(ret);
	}

	return nullptr;
}

#endif /* SLEEPLIST_H_ */
