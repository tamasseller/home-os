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

	inline bool operator < (uintptr_t time) const {
		return (intptr_t)(deadline - time) < 0;
	}

	inline bool operator < (const Sleeper& other) const {
		return *this < other.deadline;
	}

	inline bool isSleeping() {
		return prev == invalid;
	}

	inline void invalidate() {
		prev = invalid;
	}
};

template<class... Args>
class Scheduler<Args...>::SleepList:
	private pet::OrderedDoubleList<typename Scheduler<Args...>::Sleeper> {
	typedef pet::OrderedDoubleList<Sleeper> List;

public:
	inline void add(Task* elem);
	inline void remove(Task* elem);
	inline Task* peek();
	inline Task* pop();
};

template<class... Args>
inline void Scheduler<Args...>::SleepList:: add(Task* elem)
{
	List::add(static_cast<Sleeper*>(elem));
}

template<class... Args>
inline void Scheduler<Args...>::SleepList:: remove(Task* elem)
{
	Sleeper* sleeper = static_cast<Sleeper*>(elem);
	List::remove();
	sleeper->invalidate();
}

template<class... Args>
inline typename Scheduler<Args...>::Task* Scheduler<Args...>::SleepList::peek()
{
	if(Sleeper* ret = List::lowest())
		return static_cast<Task*>(ret);

	return nullptr;
}

template<class... Args>
inline typename Scheduler<Args...>::Task* Scheduler<Args...>::SleepList::pop()
{
	if(Sleeper* ret = List::popLowest()) {
		ret->invalidate();
		return static_cast<Task*>(ret);
	}

	return nullptr;
}

#endif /* SLEEPERS_H_ */
