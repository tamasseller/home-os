/*******************************************************************************
 *
 * Copyright (c) 2017 Seller Tamás. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *******************************************************************************/

#ifndef SLEEPLIST_H_
#define SLEEPLIST_H_

#include "Scheduler.h"

template<class... Args>
template<class Dummy>
class Scheduler<Args...>::SleeperBase<SchedulerOptions::ScalabilityHint::Few, Dummy> {
	using Sleeper = typename Scheduler<Args...>::Sleeper;
	using Task = typename Scheduler<Args...>::Task;
	static constexpr Sleeper *invalid() {return (Sleeper *)0xffffffff;}

	friend pet::DoubleList<Sleeper>;
	Sleeper *prev = invalid(), *next;

	void (* const wakeVirtual)(SleeperBase*);
public:
	uintptr_t deadline;

	inline bool isSleeping() {
		return prev != invalid();
	}

	inline void invalidate() {
		prev = invalid();
	}

	SleeperBase(decltype(wakeVirtual) wakeVirtual): wakeVirtual(wakeVirtual) {}

	void wake() {
		wakeVirtual(this);
	}
};

template<class... Args>
template<class Dummy>
class Scheduler<Args...>::SleepListBase<SchedulerOptions::ScalabilityHint::Few, Dummy> {
	using Sleeper = typename Scheduler<Args...>::Sleeper;
	using Profile = typename Scheduler<Args...>::Profile;

	static inline bool compareDeadline(const Sleeper& a, const Sleeper& b) {
		return a.deadline < b.deadline;
	}

	pet::OrderedDoubleList<Sleeper, &SleepListBase::compareDeadline> list;

public:
	inline void delay(Sleeper* elem, uintptr_t delay)
	{
		elem->deadline = Profile::getTick() + delay;
		list.add(elem);
	}

	inline void update(Sleeper* elem, uintptr_t delay)
	{
		list.remove(elem);
		delay(elem, delay);
	}

	inline void remove(Sleeper* elem)
	{
		list.remove(elem);
		elem->invalidate();
	}

	inline Sleeper* getWakeable() {
		if(Sleeper* ret = list.lowest()) {
			if(ret->deadline <= Profile::getTick()) {
				list.popLowest();
				ret->invalidate();
				return ret;
			}
		}
		return nullptr;
	}
};

template<class... Args>
template<class Dummy>
class Scheduler<Args...>::SleeperBase<SchedulerOptions::ScalabilityHint::Many, Dummy>: pet::HeapNode {
	using Sleeper = typename Scheduler<Args...>::Sleeper;
	using SleepList = typename Scheduler<Args...>::SleepList;
	friend SleepList;

	static constexpr pet::HeapNode* invalid() {return (pet::HeapNode*)0xffffffff;}

	void (* const wakeVirtual)(SleeperBase*);

public:
	uintptr_t deadline;

	inline bool isSleeping() {
		return parent != invalid();
	}

	inline void invalidate() {
		parent = invalid();
	}

	SleeperBase(decltype(wakeVirtual) wakeVirtual): wakeVirtual(wakeVirtual) {
		invalidate();
	}

	void wake() {
		wakeVirtual(this);
	}
};


template<class... Args>
template<class Dummy>
class Scheduler<Args...>::SleepListBase<SchedulerOptions::ScalabilityHint::Many, Dummy> {
	using Sleeper = typename Scheduler<Args...>::Sleeper;
	using Profile = typename Scheduler<Args...>::Profile;

	static constexpr inline bool compareDeadline(const pet::HeapNode* a, const pet::HeapNode* b) {
		return static_cast<const Sleeper*>(a)->deadline < static_cast<const Sleeper*>(b)->deadline;
	}

	pet::BinaryHeap<&SleepListBase::compareDeadline> heap;

public:
	inline void delay(Sleeper* elem, uintptr_t delay)
	{
		elem->deadline = Profile::getTick() + delay;
		heap.insert(elem);
	}

	inline void update(Sleeper* elem, uintptr_t delay)
	{
		elem->deadline = Profile::getTick() + delay;
		heap.update(elem);
	}

	inline void remove(Sleeper* elem)
	{
		heap.remove(elem);
		elem->invalidate();
	}

	inline Sleeper* getWakeable() {
		if(Sleeper* ret = static_cast<Sleeper*>(heap.extreme())) {
			if(ret->deadline <= Profile::getTick()) {
				heap.pop();
				ret->invalidate();
				return ret;
			}
		}
		return nullptr;
	}
};

#endif /* SLEEPLIST_H_ */
