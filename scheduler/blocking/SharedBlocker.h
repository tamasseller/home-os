/*
 * SharedBlocker.h
 *
 *  Created on: 2017.10.25.
 *      Author: tooma
 */

#ifndef SHAREDBLOCKER_H_
#define SHAREDBLOCKER_H_

#include "Scheduler.h"

/**
 * Common base for shared blocker objects.
 *
 * A shared blocker is one that multiple tasks can be blocked by,
 * this type of blocker must register all of the currently blocked
 * tasks, thus this base class includes a list of them and provides
 * an implementation for the relevant part of the Blocker interface.
 */
template<class... Args>
class Scheduler<Args...>::SharedBlocker: public Blocker
{
	virtual void priorityChanged(Blockable* b, Priority old) override {
	    /*
	     * If the priority of a blocked task is changed the ordering of
	     * the list get compromised, so the offending task is removed
	     * and then re-added which ensures its correct position in the
	     * list.
	     */
		waiters.remove(b);
		waiters.add(b);
	}

	static bool comparePriority(const Blockable& a, const Blockable& b) {
	    /*
	     * Ordering ing based on the defined priorities of the tasks.
	     */
	    return firstPreemptsSecond(a.getTask(), b.getTask());
	}

protected:
	/*
	 * Blocked task storage.
	 *
	 * This list is ordered by the priority of the tasks, so that when
	 * one is needed to be unblocked, the one with the highest priority
	 * can be selected.
	 */
	pet::OrderedDoubleList<Blockable, &SharedBlocker::comparePriority> waiters;

	virtual inline void block(Blockable* b) override {
	    /*
	     * Add the task to be blocked to the list,
	     * keeping the order of the list.
	     */
		waiters.add(b);
	}
};

#endif /* SHAREDBLOCKER_H_ */
