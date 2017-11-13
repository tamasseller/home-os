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

#ifndef WAITABLESET_H_
#define WAITABLESET_H_


template<class... Args>
class Scheduler<Args...>::WaitableSet final: Blocker, Registry<WaitableSet>::ObjectBase
{
	friend Scheduler<Args...>;
	static constexpr uintptr_t blockedReturnValue = 0;
	static constexpr uintptr_t timeoutReturnValue = 0;

	struct Waiter: Blockable {
		friend WaitableSet;
		Task* task;
		Blocker* blocker;

		static Task* getTask(Blockable* b) {
			return static_cast<Waiter*>(b)->task;
		}

		inline Waiter(): Blockable(&Waiter::getTask) {};
	};

	volatile const uintptr_t nWaiters;
	Waiter* volatile const waiters;
	uintptr_t retval;

	template<class... T>
	WaitableSet(Waiter (& waiters)[sizeof...(T)], T... blockers): nWaiters(sizeof...(T)), waiters(&waiters[0])
	{
		Blocker* const tempArray[] = {static_cast<Blocker*>(blockers)...};

		volatile Waiter* it = waiters;

		for(Blocker* blocker: tempArray)
			(*it++).blocker = blocker;

		Registry<WaitableSet>::registerObject(this);
	}

	~WaitableSet() {
		Registry<WaitableSet>::unregisterObject(this);
	}

	inline uintptr_t take(Blocker* blocker, Task* task) {
		retval = blocker->acquire(task);
		return reinterpret_cast<uintptr_t>(blocker);
	}

	virtual void canceled(Blockable* blockable, Blocker* blocker) override final
	{
		assert(blockable == waiters[0].task, ErrorStrings::unknownError);

		for(uintptr_t i = 0; i < nWaiters; i++)
			waiters[i].blocker->canceled(waiters + i, this);

		Profile::injectReturnValue(static_cast<Task*>(blockable), reinterpret_cast<uintptr_t>(blocker));
	}

	virtual void timedOut(Blockable* blockable) override final
	{
		this->WaitableSet::canceled(blockable, nullptr);
	}

	virtual void priorityChanged(Blockable*, typename Policy::Priority old) override final
	{
		for(uintptr_t i = 0; i < nWaiters; i++)
			waiters[i].blocker->priorityChanged(waiters + i, old);
	}

	virtual Blocker* getTakeable(Task* task) override final
	{
		for(uintptr_t i=0; i < nWaiters; i++) {
			if(Blocker *receiver = waiters[i].blocker->getTakeable(task))
				return receiver;
		}

		return nullptr;
	}

	virtual void block(Blockable* b) override final {
		for(uintptr_t i=0; i < nWaiters; i++) {
			Waiter *waiter = waiters + i;
			Blocker *blocker = waiter->blocker;
			waiter->task = b->getTask();
			blocker->block(waiter);
		}
	}

	virtual bool continuation(uintptr_t retval) {
		if(retval)
			return reinterpret_cast<Blocker*>(retval)->continuation(this->retval);

		return false;
	}

	/*
	 * These two methods are never called during normal operation, so
	 * LCOV_EXCL_START is placed here to exclude them from coverage analysis
	 */

	virtual uintptr_t acquire(Task*) override final {
		assert(false, ErrorStrings::unreachableReached);
		return 0;
	}

	virtual bool release(uintptr_t arg) override final {
		assert(false, ErrorStrings::unreachableReached);
		return false;
	}

	/*
	 * From here on, the rest should be check for test coverage, so
	 * LCOV_EXCL_STOP is placed here.
	 */
public:
};

template<class... Args>
template<class... T>
inline typename Scheduler<Args...>::Blocker* Scheduler<Args...>::select(T... t)
{
	typename WaitableSet::Waiter waiters[sizeof...(t)];
	WaitableSet set(waiters, t...);

	return reinterpret_cast<Blocker*>(blockOn(&set));
}

template<class... Args>
template<class... T>
inline typename Scheduler<Args...>::Blocker* Scheduler<Args...>::selectTimeout(uintptr_t timeout, T... t)
{
	typename WaitableSet::Waiter waiters[sizeof...(t)];
	WaitableSet set(waiters, t...);

	return reinterpret_cast<Blocker*>(timedBlockOn(&set, timeout));
}

#endif /* WAITABLESET_H_ */
