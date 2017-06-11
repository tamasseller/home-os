/*
 * SleepList.h
 *
 *  Created on: 2017.05.28.
 *      Author: tooma
 */

#ifndef SLEEPLIST_H_
#define SLEEPLIST_H_

#include "Scheduler.h"
#include "data/OrderedDoubleList.h"

template<class... Args>
class Scheduler<Args...>::SleepList {
	static inline bool compareDeadline(const Sleeper& a, const Sleeper& b) {
		return a.deadline < b.deadline;
	}

	pet::OrderedDoubleList<Sleeper, &SleepList::compareDeadline> list;

public:
	inline void delay(Task* elem, uintptr_t delay)
	{
		elem->deadline = Profile::Timer::getTick() + delay;
		list.add(elem);
	}

	inline void remove(Task* elem)
	{
		list.remove(elem);
		elem->invalidate();
	}

	inline Task* getWakeable() {
		if(Sleeper* ret = list.lowest()) {
			if(ret->deadline <= Profile::Timer::getTick()) {
				list.popLowest();
				ret->invalidate();
				return static_cast<Task*>(ret);
			}
		}
		return nullptr;
	}
};

#endif /* SLEEPLIST_H_ */
