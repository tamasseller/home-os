/*
 * SleepList.h
 *
 *  Created on: 2017.05.28.
 *      Author: tooma
 */

#ifndef SLEEPLIST_H_
#define SLEEPLIST_H_

#include "Scheduler.h"

template<class... Args>
template<class Dummy>
class Scheduler<Args...>::SleeperBase<SchedulerOptions::ScalabilityHint::Few, Dummy> {
	using Sleeper = typename Scheduler<Args...>::Sleeper;
	static constexpr Sleeper *invalid() {return (Sleeper *)0xffffffff;}

	friend pet::DoubleList<Sleeper>;
	Sleeper *prev = invalid(), *next;

public:
	uintptr_t deadline;

	inline bool isSleeping() {
		return prev != invalid();
	}

	inline void invalidate() {
		prev = invalid();
	}
};

template<class... Args>
template<class Dummy>
class Scheduler<Args...>::SleepListBase<SchedulerOptions::ScalabilityHint::Few, Dummy> {
	using Sleeper = typename Scheduler<Args...>::Sleeper;
	using SleepList = typename Scheduler<Args...>::SleepList;
	using Task = typename Scheduler<Args...>::Task;
	using Profile = typename Scheduler<Args...>::Profile;

	static inline bool compareDeadline(const Sleeper& a, const Sleeper& b) {
		return a.deadline < b.deadline;
	}

	pet::OrderedDoubleList<Sleeper, &SleepList::compareDeadline> list;

public:
	inline void delay(Task* elem, uintptr_t delay)
	{
		elem->deadline = Profile::getTick() + delay;
		list.add(elem);
	}

	inline void remove(Task* elem)
	{
		list.remove(elem);
		elem->invalidate();
	}

	inline Task* getWakeable() {
		if(Sleeper* ret = list.lowest()) {
			if(ret->deadline <= Profile::getTick()) {
				list.popLowest();
				ret->invalidate();
				return static_cast<Task*>(ret);
			}
		}
		return nullptr;
	}
};

#endif /* SLEEPLIST_H_ */
