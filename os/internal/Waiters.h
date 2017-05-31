/*
 * Waiters.h
 *
 *  Created on: 2017.05.30.
 *      Author: tooma
 */

#ifndef WAITERS_H_
#define WAITERS_H_

#include "Scheduler.h"

#include "data/OrderedDoubleList.h"

template<class... Args>
class Scheduler<Args...>::Waiter: public Policy::Priority {
	static constexpr Waiter *invalid = (Waiter *)0xffffffff;

	friend pet::DoubleList<Waiter>;

	Waiter *prev = invalid, *next;

	friend WaitList;
	inline void invalidate() {
		prev = invalid;
	}

public:
	inline bool isWaiting() {
		return prev == invalid;
	}

	inline Task *prepareWakeup() {
		return prepareWakeupCallback(this);
	}
};

template<class... Args>
class Scheduler<Args...>::WaitList:
	private pet::OrderedDoubleList<typename Scheduler<Args...>::Waiter>
{
	typedef pet::OrderedDoubleList<Waiter> List;
public:
	inline void add(Task* elem)
	{
		List::add(static_cast<Waiter*>(elem));
	}

	inline void remove(Task* elem)
	{
		Waiter* waiter = elem;
		List::remove(waiter);
		waiter->invalidate();
	}

	inline Task* peek()
	{
		if(Waiter* ret = List::lowest())
			return static_cast<Task*>(ret);

		return nullptr;
	}

	inline Task* pop()
	{
		if(Waiter* ret = List::popLowest()) {
			ret->invalidate();
			return static_cast<Task*>(ret);
		}

		return nullptr;
	}
};

#endif /* WAITERS_H_ */
