/*
 * Waitable.h
 *
 *  Created on: 2017.05.30.
 *      Author: tooma
 */

#ifndef WAITABLE_H_
#define WAITABLE_H_

#include "Scheduler.h"

template<class... Args>
class Scheduler<Args...>::Waitable: Blocker, Event {
	friend Scheduler<Args...>;
protected:

	/**
	 * Wake up a task if there is any waiting.
	 *
	 * @return	True if a task was waken, false otherwise.
	 */
	bool wakeOne()
	{
		if(Blockable* blockable = waiters.lowest()) {
			Task* waken = blockable->getTask();

			if(waken->isSleeping())
				state.sleepList.remove(waken);

			waken->blockedBy->waken(waken, this);

			state.policy.addRunnable(waken);

			return true;
		}

		return false;
	}

private:
    /*
     * Implementation defined method to determine if
     * the synchronization object is blocking during
     * the acquire/lock/wait/pend call.
     *
	 * @return	Must return true if there the caller
	 * 			needs to be blocked, i.e. if the acquiring
	 * 			can not be successful at the moment, if
	 * 			returns true, the caller is blocked and
	 * 			put to the waiting list.
	 */
	virtual bool wouldBlock() = 0;

	/**
	 * Implementation defined acquire/lock/wait/pend method.
	 *
	 * @note	It is only called after establishing
	 * 			that it is going to be successfull by
	 * 			querying via the _wouldBlock_ method.
	 */
	virtual void acquire() = 0;

	/**
	 * Implementation defined release/unlock/notify/send
	 * method.
	 *
	 * @param	session The WakeSession that can be used
	 * 					to wake up waiters.
	 *
	 * @param	arg		The number of deferred release calls,
	 * 					the argument retrieved from the event
	 * 					list. See the description of EventList
	 * 					and AtomicList for details.
	 */
	virtual void release(uintptr_t arg) = 0;

	virtual void remove(Task* task) final {
		waiters.remove(task);
		Profile::injectReturnValue(task, false);
	}

	virtual void waken(Task* task, Waitable*) final {
		waiters.remove(task);
	}

	virtual void priorityChanged(Task* task, Priority old) {
		waiters.remove(task);
		waiters.add(task);
	}

	static bool comparePriority(const Blockable& a, const Blockable& b) {
		return firstPreemptsSecond(a.getTask(), b.getTask());
	}

	pet::OrderedDoubleList<Blockable, &Waitable::comparePriority> waiters;

	static void doNotify(Event* event, uintptr_t arg)
	{
		Waitable* waitable = static_cast<Waitable*>(event);
		Task* currentTask = static_cast<Task*>(Profile::getCurrent());

		waitable->release(arg);

		if(Task* newTask = static_cast<Task*>(state.policy.peekNext())) {
			if(!currentTask || firstPreemptsSecond(newTask, currentTask))
				switchToNext<true>();
		}
	}

public:
	inline void init() {}

	inline void wait()
	{
		Profile::sync(&Scheduler<Args...>::doWait, detypePtr(this));
	}

	inline bool wait(uintptr_t timeout) {
		return Profile::sync(&Scheduler<Args...>::doWaitTimeout, detypePtr(this), timeout);
	}

	inline void notify() {
		state.eventList.issue(this);
	}

	inline Waitable(): Event(doNotify) {}
};

template<class... Args>
uintptr_t Scheduler<Args...>::doWait(uintptr_t waitablePtr)
{
	Waitable* waitable = entypePtr<Waitable>(waitablePtr);
	Task* currentTask = static_cast<Task*>(Profile::getCurrent());

	if(waitable->wouldBlock()) {
		waitable->waiters.add(currentTask);
		currentTask->blockedBy = waitable;
		switchToNext<false>();
	} else
		waitable->acquire();

	return true;
}

template<class... Args>
uintptr_t Scheduler<Args...>::doWaitTimeout(uintptr_t waitablePtr, uintptr_t timeout)
{
	Waitable* waitable = entypePtr<Waitable>(waitablePtr);
	Task* currentTask = static_cast<Task*>(Profile::getCurrent());

	if(!waitable->wouldBlock()) {
		waitable->acquire();
		return true;
	}

	if(!timeout)
		return false;

	waitable->waiters.add(currentTask);
	currentTask->blockedBy = waitable;
	state.sleepList.delay(currentTask, timeout);
	switchToNext<false>();

	/*
	 * False return value will be injected by
	 * the tick handler, if timed out.
	 */
	return true;
}

#endif /* WAITABLE_H_ */
