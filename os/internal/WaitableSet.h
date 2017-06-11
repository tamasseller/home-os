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

		static Task* getTask(Blockable* b);
		inline Waiter(): Blockable(&Waiter::getTask) {};
	};

	volatile const uintptr_t nWaiters;
	Waiter* volatile const waiters;

	template<class... T>
	WaitableSet(Waiter (& waiters)[sizeof...(T)], T... waitables);

	virtual void remove(Task* task);
	virtual void waken(Task* task, Waitable* source);
};

template<class... Args>
typename Scheduler<Args...>::Task* Scheduler<Args...>::WaitableSet::Waiter::getTask(Blockable* b) {
	return static_cast<Waiter*>(b)->task;
}

template<class... Args>
template<class... T>
Scheduler<Args...>::WaitableSet::WaitableSet(Waiter (& waiters)[sizeof...(T)], T... waitables):
	waiters(&waiters[0]), nWaiters(sizeof...(T))
{
	Waitable* const tempArray[] = {static_cast<Waitable*>(waitables)...};
	volatile Waiter* it = waiters;
	for(Waitable* waitable: tempArray)
		(*it++).waitable = waitable;
}

template<class... Args>
void Scheduler<Args...>::WaitableSet::remove(Task* task)
{
	for(uintptr_t i = 0; i < nWaiters; i++)
		waiters[i].waitable->waiters.remove(waiters + i);
}

template<class... Args>
void Scheduler<Args...>::WaitableSet::waken(Task* task, Waitable* source)
{
	for(uintptr_t i = 0; i < nWaiters; i++)
		waiters[i].waitable->waiters.remove(waiters + i);

	task->injectReturnValue(detypePtr(source));
}

template<class... Args>
template<class... T>
inline typename Scheduler<Args...>::Waitable* Scheduler<Args...>::select(T... t)
{
	using CallGate = typename Profile::CallGate;

	typename WaitableSet::Waiter waiters[sizeof...(t)];
	WaitableSet set(waiters, t...);

	auto ret = CallGate::sync(&Scheduler<Args...>::doSelect, detypePtr(&set));

	return entypePtr<Waitable>(ret);
}

template<class... Args>
uintptr_t Scheduler<Args...>::doSelect(uintptr_t waitableSet){
	WaitableSet *set = entypePtr<WaitableSet>(waitableSet);
	Task* currentTask = static_cast<Task*>(Profile::Task::getCurrent());

	for(uintptr_t i=0; i < set->nWaiters; i++) {
		Waitable *waitable = set->waiters[i].waitable;
		if(!waitable->wouldBlock()) {
			waitable->acquire();
			return detypePtr(waitable);
		}
	}

	currentTask->waitsFor = set;

	for(uintptr_t i=0; i < set->nWaiters; i++) {
		typename WaitableSet::Waiter *waiter = set->waiters + i;
		Waitable *waitable = waiter->waitable;
		waiter->task = currentTask;
		waitable->waiters.add(waiter);
	}

	switchToNext<false>();

	/*
	 * The waker will inject the actual return value.
	 */
	return 0;
}

#endif /* WAITABLESET_H_ */
