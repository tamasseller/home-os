/*
 * Sleepers.h
 *
 *  Created on: 2017.05.28.
 *      Author: tooma
 */

#ifndef SLEEPERS_H_
#define SLEEPERS_H_

#include "Scheduler.h"
#include "data/OrderedDoubleList.h"


template<class... Args>
class Scheduler<Args...>::Sleeper {
	static constexpr Sleeper *invalid = (Sleeper *)0xffffffff;

	friend pet::DoubleList<Sleeper>;
	Sleeper *prev = invalid, *next;

public:
	uintptr_t deadline;

	inline bool isSleeping() {
		return prev != invalid;
	}

	inline void invalidate() {
		prev = invalid;
	}
};

template<class... Args>
class Scheduler<Args...>::SleepList {
	static inline bool compareDeadline(const Sleeper& a, const Sleeper& b);
	pet::OrderedDoubleList<Sleeper, &SleepList::compareDeadline> list;

public:
	inline void delay(Task* elem, uintptr_t delay);
	inline void remove(Task* elem);
	inline Task* getWakeable();
};

template<class... Args>
inline bool Scheduler<Args...>::SleepList::compareDeadline(const Sleeper& a, const Sleeper& b) {
	return a.deadline < b.deadline;
}


template<class... Args>
inline void Scheduler<Args...>::SleepList::delay(Task* elem, uintptr_t delay)
{
	elem->deadline = Profile::Timer::getTick() + delay;
	list.add(static_cast<Sleeper*>(elem));
}

template<class... Args>
inline void Scheduler<Args...>::SleepList::remove(Task* elem)
{
	list.remove(elem);
	elem->invalidate();
}

template<class... Args>
inline typename Scheduler<Args...>::Task* Scheduler<Args...>::SleepList::getWakeable()
{
	if(Sleeper* ret = list.lowest()) {
		if(ret->deadline <= Profile::Timer::getTick()) {
			list.popLowest();
			ret->invalidate();
			return static_cast<Task*>(ret);
		}
	}
	return nullptr;
}


#endif /* SLEEPERS_H_ */
