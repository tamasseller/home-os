/*
 * SleepList.h
 *
 *  Created on: 2017.05.28.
 *      Author: tooma
 */

#ifndef SLEEPLIST_H_
#define SLEEPLIST_H_

#include "Task.h"
#include "data/OrderedDoubleList.h"

template<class Profile, template<class> class Policy>
class Scheduler<Profile, Policy>::SleepList:
	private pet::OrderedDoubleList<typename Scheduler<Profile, Policy>::Sleeper> {
public:
	inline void add(TaskBase* elem);
	inline void remove(TaskBase* elem);
	inline TaskBase* peek();
	inline TaskBase* pop();
};

template<class Profile, template<class> class Policy>
inline void Scheduler<Profile, Policy>::SleepList:: add(TaskBase* elem)
{
	this->pet::OrderedDoubleList<Sleeper>::add(static_cast<Sleeper*>(elem));
}

template<class Profile, template<class> class Policy>
inline void Scheduler<Profile, Policy>::SleepList:: remove(TaskBase* elem)
{
	this->pet::OrderedDoubleList<Sleeper>::remove(static_cast<Sleeper*>(elem));
}

template<class Profile, template<class> class Policy>
inline typename Scheduler<Profile, Policy>::TaskBase* Scheduler<Profile, Policy>::SleepList::peek()
{
	if(Sleeper* ret = this->pet::OrderedDoubleList<Sleeper>::lowest())
		return static_cast<TaskBase*>(ret);

	return nullptr;
}

template<class Profile, template<class> class Policy>
inline typename Scheduler<Profile, Policy>::TaskBase* Scheduler<Profile, Policy>::SleepList::pop()
{
	if(Sleeper* ret = this->pet::OrderedDoubleList<Sleeper>::popLowest())
		return static_cast<TaskBase*>(ret);

	return nullptr;
}

#endif /* SLEEPLIST_H_ */
