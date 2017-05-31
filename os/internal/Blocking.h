/*
 * Blocking.h
 *
 *  Created on: 2017.05.30.
 *      Author: tooma
 */

#ifndef BLOCKING_H_
#define BLOCKING_H_

#include "Scheduler.h"

#include "data/OrderedDoubleList.h"

template<class... Args>
class Scheduler<Args...>::Blockable: public Policy::Priority {
	static constexpr Blockable *invalid = (Blockable *)0xffffffff;

	friend pet::DoubleList<Blockable>;

	Blockable *prev = invalid, *next;

	friend BlockableList;
	inline void invalidate() {
		prev = invalid;
	}

public:
	inline bool isBlockable() {
		return prev == invalid;
	}

	inline Task *prepareWakeup() {
		return prepareWakeupCallback(this);
	}
};

template<class... Args>
class Scheduler<Args...>::BlockableList:
	private pet::OrderedDoubleList<typename Scheduler<Args...>::Blockable>
{
	typedef pet::OrderedDoubleList<Blockable> List;
public:
	inline void add(Task* elem)
	{
		List::add(static_cast<Blockable*>(elem));
	}

	inline void remove(Task* elem)
	{
		Blockable* waiter = elem;
		List::remove(waiter);
		waiter->invalidate();
	}

	inline Task* peek()
	{
		if(Blockable* ret = List::lowest())
			return static_cast<Task*>(ret);

		return nullptr;
	}

	inline Task* pop()
	{
		if(Blockable* ret = List::popLowest()) {
			ret->invalidate();
			return static_cast<Task*>(ret);
		}

		return nullptr;
	}
};

#endif /* BLOCKING_H_ */
