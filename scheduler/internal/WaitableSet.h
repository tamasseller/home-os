/*
 * WaitableSet.h
 *
 *  Created on: 2017.06.05.
 *      Author: tooma
 */

#ifndef WAITABLESET_H_
#define WAITABLESET_H_


template<class... Args>
class Scheduler<Args...>::WaitableSet final: Blocker
{
	friend Scheduler<Args...>;

	struct Waiter: Blockable {
		friend WaitableSet;
		Task* task;
		Waitable* waitable;

		static Task* getTask(Blockable* b) {
			return static_cast<Waiter*>(b)->task;
		}

		inline Waiter(): Blockable(&Waiter::getTask) {};
	};

	volatile const uintptr_t nWaiters;
	Waiter* volatile const waiters;

	template<class... T>
	WaitableSet(Waiter (& waiters)[sizeof...(T)], T... waitables): nWaiters(sizeof...(T)), waiters(&waiters[0])
	{
		Waitable* const tempArray[] = {static_cast<Waitable*>(waitables)...};

		volatile Waiter* it = waiters;

		for(Waitable* waitable: tempArray)
			(*it++).waitable = waitable;
	}

	virtual void remove(Task* task)
	{
		for(uintptr_t i = 0; i < nWaiters; i++)
			waiters[i].waitable->waiters.remove(waiters + i);
	}

	virtual void waken(Task* task, Waitable* source)
	{
		for(uintptr_t i = 0; i < nWaiters; i++)
			waiters[i].waitable->waiters.remove(waiters + i);

		Profile::injectReturnValue(task, detypePtr(source));
	}

	virtual void priorityChanged(Task*, Priority old)
	{
		for(uintptr_t i = 0; i < nWaiters; i++) {
			waiters[i].waitable->waiters.remove(waiters + i);
			waiters[i].waitable->waiters.add(waiters + i);
		}
	}

	Waitable* acquireAny()
	{
		for(uintptr_t i=0; i < nWaiters; i++) {
			Waitable *waitable = waiters[i].waitable;
			if(!waitable->wouldBlock()) {
				waitable->acquire();
				return waitable;
			}
		}

		return nullptr;
	}

	void blockOnAll(Task* task)
	{
		task->blockedBy = this;

		for(uintptr_t i=0; i < nWaiters; i++) {
			Waiter *waiter = waiters + i;
			Waitable *waitable = waiter->waitable;
			waiter->task = task;
			waitable->waiters.add(waiter);
		}
	}
};

template<class... Args>
template<class... T>
inline typename Scheduler<Args...>::Waitable* Scheduler<Args...>::select(T... t)
{
	typename WaitableSet::Waiter waiters[sizeof...(t)];
	WaitableSet set(waiters, t...);

	auto ret = Profile::sync(&Scheduler<Args...>::doSelect, detypePtr(&set));

	return entypePtr<Waitable>(ret);
}

template<class... Args>
template<class... T>
inline typename Scheduler<Args...>::Waitable* Scheduler<Args...>::selectTimeout(uintptr_t timeout, T... t)
{
	typename WaitableSet::Waiter waiters[sizeof...(t)];
	WaitableSet set(waiters, t...);

	auto ret = Profile::sync(&Scheduler<Args...>::doSelectTimeout, detypePtr(&set), timeout);

	return entypePtr<Waitable>(ret);
}

template<class... Args>
uintptr_t Scheduler<Args...>::doSelect(uintptr_t waitableSet){
	WaitableSet *set = entypePtr<WaitableSet>(waitableSet);
	Task* currentTask = static_cast<Task*>(Profile::getCurrent());

	if(auto ret = set->acquireAny())
		return detypePtr(ret);

	set->blockOnAll(currentTask);

	switchToNext<false>();

	/*
	 * The waker will inject the actual return value.
	 */
	return 0;
}


template<class... Args>
uintptr_t Scheduler<Args...>::doSelectTimeout(uintptr_t waitableSet, uintptr_t timeout){
	WaitableSet *set = entypePtr<WaitableSet>(waitableSet);
	Task* currentTask = static_cast<Task*>(Profile::getCurrent());

	if(auto ret = set->acquireAny())
		return detypePtr(ret);

	if(timeout) {
		set->blockOnAll(currentTask);
		state.sleepList.delay(currentTask, timeout);
		switchToNext<false>();
	}

	/*
	 * The waker will inject the actual return value.
	 */
	return 0;
}

#endif /* WAITABLESET_H_ */
