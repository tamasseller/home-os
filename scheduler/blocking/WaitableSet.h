/*
 * WaitableSet.h
 *
 *  Created on: 2017.06.05.
 *      Author: tooma
 */

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

	static inline uintptr_t take(Blocker* blocker, Task* task) {
		blocker->acquire(task);
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

	auto ret = syscall<SYSCALL(doBlock<WaitableSet>)>(Registry<WaitableSet>::getRegisteredId(&set));

	return reinterpret_cast<Blocker*>(ret);
}

template<class... Args>
template<class... T>
inline typename Scheduler<Args...>::Blocker* Scheduler<Args...>::selectTimeout(uintptr_t timeout, T... t)
{
	typename WaitableSet::Waiter waiters[sizeof...(t)];
	WaitableSet set(waiters, t...);

	auto ret = syscall<SYSCALL(doTimedBlock<WaitableSet>)>(Registry<WaitableSet>::getRegisteredId(&set), timeout);

	return reinterpret_cast<Blocker*>(ret);
}

#endif /* WAITABLESET_H_ */
