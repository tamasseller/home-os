/*
 * Blocker.h
 *
 *  Created on: 2017.06.02.
 *      Author: tooma
 */

#ifndef BLOCKER_H_
#define BLOCKER_H_

#include "Scheduler.h"

template<class... Args>
class Scheduler<Args...>::Blocker
{
	friend Scheduler<Args...>;

	/**
	 * Remove a blocked element.
	 *
	 * There are two distinct scenarios that can lead to calling this:
	 *
	 *  1. If the second argument is null: a timeout occured and the tick
	 *     handler called this method.
	 *  2. If the second argument is not null: a regular release-type
	 *     event triggers the release of the blocked task.
	 */
	virtual void remove(Blockable* blockable, Blocker* blocker) = 0;

	/**
	 * Update the internal ordering.
	 *
	 * Mutex priority inversion avoidance or explicit priority overriding
	 * can trigger calling this method.
	 */
	virtual void priorityChanged(Blockable* blockable, Priority oldPrio) = 0;

	/**
	 * Tell how to take the
	 *
	 * For the regular blockers this method either retuns null or _this_.
	 */
	virtual Blocker* getTakeable(Task*) = 0;

	/**
	 * Tell how to take the blocker.
	 *
	 * This method is compile time polymorphic! In the common case this is
	 * a simple fall-through call to the acquire method of the _blocker_.
	 *
	 * Although, for the simultaneous multiple waiting scneario it is used
	 * to override the default return value of the blocker that actually
	 * takes _task_.
	 */
	static inline uintptr_t take(Blocker* blocker, Task* task) {
		return blocker->acquire(task);
	}

	/**
	 * Acquire/lock.
	 *
	 * This method is used to take the resource or register the _task_  as
	 * the owner of if it is already capable of being taken.
	 *
	 * @NOTE This method is only called on a blocker that reported that it
	 *       is in the ready state, via the getTakeable method.
	 */
	virtual uintptr_t acquire(Task* task) = 0;

	/**
	 * Add an entry to the waiters' list.
	 *
	 * This method is with two kinds of Blockables:
	 *
	 *  1. Regular tasks, for single waiter setups.
	 *  2. WaitableSet::Waiter objects, in multiple waiting cases.
	 */
	virtual void block(Blockable*) = 0;

	/**
	 * Release/unlock.
	 *
	 * This method can be called from two different contexts:
	 *
	 *  1. From a synchronous release call, then _n_ is zero.
	 *  2. From a cumulated asynchronous event handler, then
	 *     _n_ is the number of triggers.
	 */
	virtual bool release(uintptr_t n) = 0;

	/**
	 * Return value for timed out request.
	 *
	 * This value gets returned when timed blocking request fail to
	 * acquire the resource and the timeout value is specified as zero.
	 */
	static constexpr uintptr_t timeoutReturnValue = 0;

	/**
	 * Return value for a blocked request.
	 *
	 * This value gets written onto the task's stack when it is blocked,
	 * because of the unavailability of the requested resource.
	 */
	static constexpr uintptr_t blockedReturnValue = 1;
};

template<class... Args>
template<class ActualBlocker>
uintptr_t Scheduler<Args...>::doBlock(uintptr_t blockerPtr)
{
	ActualBlocker* blocker = entypePtr<ActualBlocker>(blockerPtr);
	Task* currentTask = static_cast<Task*>(Profile::getCurrent());

	if(Blocker* receiver = blocker->ActualBlocker::getTakeable(currentTask)) {
		return ActualBlocker::take(receiver, currentTask);
	} else {
		blocker->ActualBlocker::block(currentTask);
		currentTask->blockedBy = blocker;
		switchToNext<false>();
	}

	static constexpr uintptr_t timeoutReturnValue = 0;
	static constexpr uintptr_t blockedReturnValue = 1;

	return ActualBlocker::blockedReturnValue;
}

template<class... Args>
template<class ActualBlocker>
uintptr_t Scheduler<Args...>::doTimedBlock(uintptr_t blockerPtr, uintptr_t timeout)
{
	ActualBlocker* blocker = entypePtr<ActualBlocker>(blockerPtr);
	Task* currentTask = static_cast<Task*>(Profile::getCurrent());

	if(Blocker* receiver = blocker->ActualBlocker::getTakeable(currentTask)) {
		return ActualBlocker::take(receiver, currentTask);
	}

	if(!timeout)
		return ActualBlocker::timeoutReturnValue;

	blocker->ActualBlocker::block(currentTask);
	currentTask->blockedBy = blocker;
	state.sleepList.delay(currentTask, timeout);
	switchToNext<false>();

	/*
	 * Other return value can be injected by the remove handler.
	 */
	return ActualBlocker::blockedReturnValue;
}

template<class... Args>
template<class ActualBlocker>
uintptr_t Scheduler<Args...>::doRelease(uintptr_t blockerPtr)
{
	ActualBlocker* blocker = entypePtr<ActualBlocker>(blockerPtr);

	if(blocker->ActualBlocker::release(0))
		switchToNext<true>();

	return 0;
}


#endif /* BLOCKER_H_ */
