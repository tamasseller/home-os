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
	virtual void priorityChanged(Blockable* b, typename Policy::Priority old) override {
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
	     * Ordering based on the defined priorities of the tasks.
	     */
	    return a.getTask()->getPriority() < b.getTask()->getPriority();
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
