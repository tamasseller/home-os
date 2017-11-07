/*
 * Blocker.h
 *
 *  Created on: 2017.06.02.
 *      Author: tooma
 */

#ifndef BLOCKER_H_
#define BLOCKER_H_

#include "Scheduler.h"

/**
 * Common interface for objects that can block a task.
 *
 * Most of the methods of this interface are hooks used in various operations,
 * emerge from generalization of common operations. Some of them have no intuitive
 * meaning but are there to enable special operations.
 *
 * This interface is **not a pure runtime polymorphic** one.
 *
 * Some of the methods are intended to be called virtually in the usual way, but
 * some of them are used through de-virtualized calls (see _doBlock_, _doTimedBlock_,
 * _doRelease_). And there is one that is actually compile overridable time only.
 */
template<class... Args>
class Scheduler<Args...>::Blocker
{
	friend Scheduler<Args...>;

	/**
	 * Remove a blocked element, due to cancellation.
	 *
	 * The receiver should only remove the element without any further
	 * processing, because the caller is responsible for handling the
	 * return value. This done like this to enable overwriting the
	 * default behavior of the individual blockers in a multi-wait
	 * scenario, regarding the value returned.
	 *
	 * @NOTE This is a usual virtual method, only called through regular
	 *       virtual method dispatching.
	 */
	virtual void canceled(Blockable* blockable, Blocker* blocker) = 0;

	/**
	 * Remove a blocked element, due to timeout.
	 *
	 * If a timeout occurs this method gets called, the receiver shall
	 * fill in the return value for the task in question as specified
	 * for a timeout event and remove it from the waiters queue.
	 *
	 * @NOTE This is a usual virtual method, only called through regular
	 *       virtual method dispatching.
	 */
	virtual void timedOut(Blockable* blockable) = 0;

	/**
	 * Update the internal ordering.
	 *
	 * When the mutex priority inversion avoidance mechanism or an explicit
	 * priority overriding operation changes a blocked tasks priority then
	 * the blocker that the task is blocked by (see _Task::blockedBy_) is
	 * notified via this method in order to enable it to update its internal
	 * storage of blocked tasks.
	 *
	 * @NOTE This is a usual virtual method, only called through regular
     *       virtual method dispatching.
	 */
	virtual void priorityChanged(Blockable* blockable, typename PolicyBase::Priority oldPrio) = 0;

	/**
	 * Get a (possibly indirect) blocker that can be acquired.
	 *
	 * This method is a good example of non-intuitive generalization, if
	 * there was no multi-waiter mechanism then it would be a bool valued
	 * method that would report if the blocker can be taken or not. But
	 * for the _WaitableSet_ it is better to report either one of the
	 * blockers in the set or null if all of them is blocking. For the
	 * regular blockers this method either retuns null or _this_.
	 *
     * @NOTE This is a dual use method, called in the de-virtualized form
     *       most of the time. The _WaitableSet_ is an exception.
	 */
	virtual Blocker* getTakeable(Task*) = 0;

	/**
	 * Tell how to take the blocker.
	 *
	 * In the common case this is a simple fall-through call to the acquire method of the _blocker_.
	 *
	 * Although, for the simultaneous multiple waiting scneario it is used
	 * to override the default return value of the blocker that actually
	 * takes _task_.
	 *
	 * @NOTE This method is compile time polymorphic!
	 */
	static inline uintptr_t take(Blocker* blocker, Task* task) {
		return blocker->acquire(task);
	}

	/**
	 * Synchronous acquire/lock.
	 *
	 * This method is used to take the resource or register the _task_  as
	 * the owner of if it is already capable of being taken.
	 *
	 * @return The value returned is either passed to the task that
	 *         requested the operation or discarded depending on the
	 *         _take_ implementation of the blocker that actually
	 *         initiated the operation (see _WaitableSet_).
	 *
	 * @NOTE This method is only called on a blocker that reported that it
	 *       is in the ready state, via the getTakeable method.
	 *
     * @NOTE This is a dual use method, called in the de-virtualized form
     *       most of the time. The _WaitableSet_ is an exception.
	 */
	virtual uintptr_t acquire(Task* task) = 0;

	/**
	 * Add an entry to the waiters' list.
	 *
	 * This method is with two kinds of Blockables:
	 *
	 *  1. Regular tasks, for single waiter setups.
	 *  2. WaitableSet::Waiter objects, in multiple waiting cases.
	 *
	 * @NOTE This is a dual use method, called in the de-virtualized form
     *       most of the time. The _WaitableSet_ is an exception.
	 */
	virtual void block(Blockable*) = 0;

	/**
	 * Release/unlock.
	 *
	 * This method can be called from two different contexts:
	 *
	 *  1. From a synchronous release call, then _n_ is one.
	 *  2. From a cumulated asynchronous event handler, then
	 *     _n_ is the number of triggers.
	 *
	 * @return The method should return true if another task is unblocked
	 *         and there is possibility that the current task needs to be
	 *         switched out.
	 *
	 * @NOTE This is a dual use method, called in the de-virtualized form
     *       most of the time. The _WaitableSet_ is an exception.
	 */
	virtual bool release(uintptr_t n) = 0;

	/**
	 * Return the owner if it is applicable to this blocker and exists.
	 *
	 * For example, a mutex has an explicit owner when locked, in this
	 * case it returns the owner who has the lock. Synchronization
	 * objects that have no concept of owner shall return null.
	 *
	 * @NOTE This method is used through virtual calls by the deadlock
	 *       detection mechanism (when it is enabled).
	 */
	virtual Task* getOwner() {return nullptr;}

	/**
	 * Return value for timed out request.
	 *
	 * This value gets returned when timed blocking request fail to
	 * acquire the resource and the timeout value is specified as zero.
	 *
	 * @NOTE This field is included for documentation purposes only!
	 *       It is part of the compile time interface. Implementors
	 *       should provide a compile-time constant static member.
	 */
	static uintptr_t timeoutReturnValue;

	/**
	 * Return value for a blocked request.
	 *
	 * This value gets written onto the task's stack when it is blocked,
	 * because of the unavailability of the requested resource.
	 *
	 * @NOTE This field is included for documentation purposes only!
     *       It is part of the compile time interface. Implementors
     *       should provide a compile-time constant static member.
	 */
	static uintptr_t blockedReturnValue;
};

/**
 * Block the current task on blocker indefinitely.
 *
 * This is a common method that could be used as a system call for any
 * type of real blocker, including: mutexes, semaphores, and _WaitableSets_.
 *
 * @NOTE This method uses de-virtualized calls for performance reasons.
 */
template<class... Args>
template<class ActualBlocker>
uintptr_t Scheduler<Args...>::doBlock(uintptr_t blockerPtr)
{
	ActualBlocker* blocker = Registry<ActualBlocker>::lookup(blockerPtr);
	Task* currentTask = getCurrentTask();

    /*
     * Check if the blocker can be acquired synchronously:
     *
     *  - if it can, then take it and return immediately;
     *  - if not, then block the task on the blocker.
     */
	if(Blocker* receiver = blocker->ActualBlocker::getTakeable(currentTask)) {
		return ActualBlocker::take(receiver, currentTask);
	} else {
		blocker->ActualBlocker::block(currentTask);
		currentTask->blockedBy = blocker;

	    /*
	     * The caller task (which is the current one) is blocked,
	     * a new one needs to be switched in.
	     */
		switchToNext<false>();
	}

    /*
     * Different return value can be injected by the remove
     * handler, once the task is unblocked.
     */
	return ActualBlocker::blockedReturnValue;
}

/**
 * Block the current task on blocker with timeout.
 *
 * This is a common method that could be used as a system call for any
 * type of real blocker, including: mutexes, semaphores, and _WaitableSets_.
 *
 * @NOTE This method uses de-virtualized calls for performance reasons.
 */
template<class... Args>
template<class ActualBlocker>
uintptr_t Scheduler<Args...>::doTimedBlock(uintptr_t blockerPtr, uintptr_t timeout)
{
	ActualBlocker* blocker = Registry<ActualBlocker>::lookup(blockerPtr);
	Task* currentTask = getCurrentTask();

	/*
	 * Check if the blocker can be acquired synchronously:
	 *
	 *  - if it can, then take it and return immediately;
	 *  - if not, then block the task on the blocker.
	 */
	if(Blocker* receiver = blocker->ActualBlocker::getTakeable(currentTask)) {
		return ActualBlocker::take(receiver, currentTask);
	}

	/*
	 * At this point, blocking is necessary, but if the specified
	 * timeout value is zero, then report a timeout immediately.
	 */
	if(!timeout)
		return ActualBlocker::timeoutReturnValue;

	/*
	 * The task needs to be blocked on the blocker and also needs
	 * to be installed in the sleeper list for handling the timeout.
	 */
	blocker->ActualBlocker::block(currentTask);
	currentTask->blockedBy = blocker;
	state.sleepList.delay(currentTask, timeout);

	/*
	 * The caller task (which is the current one) is blocked,
	 * a new one needs to be switched in.
	 */
	switchToNext<false>();

    /*
     * Different return value can be injected by the remove
     * handler, once the task is unblocked.
     */
	return ActualBlocker::blockedReturnValue;
}

/**
 * Block the current task on blocker with timeout.
 *
 * This is a common method that could be used as a system call for any
 * type of real blocker, including: mutexes, semaphores, and _WaitableSets_.
 *
 * @NOTE This method uses de-virtualized calls for performance reasons.
 */
template<class... Args>
template<class ActualBlocker>
uintptr_t Scheduler<Args...>::doRelease(uintptr_t blockerPtr, uintptr_t arg)
{
	ActualBlocker* blocker = Registry<ActualBlocker>::lookup(blockerPtr);
    Task* currentTask = getCurrentTask();

    if(blocker->ActualBlocker::release(arg)) {
        if (Task *newTask = state.policy.peekNext()) {
            if (!currentTask || (newTask->getPriority() < currentTask->getPriority()))
                switchToNext<true>();
        }
    }

	return 0;
}


#endif /* BLOCKER_H_ */
