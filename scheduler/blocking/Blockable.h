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

#ifndef BLOCKABLE_H_
#define BLOCKABLE_H_

#include "Scheduler.h"

namespace home {

/**
 * An object that can stand for a task in a blocking queue.
 *
 * It serves two purposes:
 *
 *  - It is container of the _next_ and _prev_ pointers for
 *    the doubly linked list used by the Blocker to queue
 *    the blocked items.
 *  - It provides a method the query the blocked task that
 *    it represents.
 */
template<class... Args>
class Scheduler<Args...>::Blockable
{

	/**
	 * Friendship is needed to allow access only for the
	 * doubly linked list selectively.
	 */
	friend pet::DoubleList<Blockable>;

	/**
	 * The prev and next pointers used by the list.
	 */
	Blockable *prev, *next;

	/**
	 * Virtual method pointer, this ad-hoc solution for runtime
	 * polymorphism is justified by the added performance,
	 * gained by avoiding one of the two indirections of the
	 * regular vtable method.
	 */
	Task* (* const getTaskVirtual)(Blockable*);

protected:

	/**
	 * Virtual pointer initializer constructor.
	 *
	 * Subclasses must use this constructor to fill the
	 * virtual method pointer.
	 */
	inline Blockable(Task* (*getTaskVirtual)(Blockable*)): getTaskVirtual(getTaskVirtual) {}

public:

	/**
	 * Get the blocked task.
	 *
	 * This method uses the virtual method pointer to
	 * get the task that this blocker object stands for.
	 */
	inline const Task *getTask() const {
		/*
		 * The reason for **const_cast** here is that if
		 * it was not used then two virtual calls would
		 * be required, which means that either the normal
		 * vptr+vtable based polymorphism or a second
		 * pointer filed would have to be used.
		 *
		 * Also for the const case the return value is
		 * casted back to const, and knowing the anticpated
		 * semantics of the virtual method it is harmless.
		 */
		return getTaskVirtual(const_cast<Blockable*>(this));
	}

	/// @see Above.
	inline Task *getTask() {
		return getTaskVirtual(this);
	}

	void wake(Blocker* blocker) {
		Task* waken = getTask();

		if(waken->isSleeping())
			state.sleepList.remove(waken);

		waken->blockedBy->canceled(waken, blocker);

		state.policy.addRunnable(waken);
	}
};

}

#endif /* BLOCKABLE_H_ */
