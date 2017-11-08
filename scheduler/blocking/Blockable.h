/*
 * Blockable.h
 *
 *  Created on: 2017.05.30.
 *      Author: tooma
 */

#ifndef BLOCKABLE_H_
#define BLOCKABLE_H_

#include "Scheduler.h"

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

#endif /* BLOCKABLE_H_ */
