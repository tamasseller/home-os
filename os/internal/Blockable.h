/*
 * Blockable.h
 *
 *  Created on: 2017.05.30.
 *      Author: tooma
 */

#ifndef BLOCKABLE_H_
#define BLOCKABLE_H_

#include "Scheduler.h"

#include "data/OrderedDoubleList.h"

template<class... Args>
class Scheduler<Args...>::Blockable
{
	friend pet::DoubleList<Blockable>;

	Blockable *prev, *next;

	Task* (* const getTaskVirtual)(Blockable*);

protected:
	inline Blockable(Task* (*getTaskVirtual)(Blockable*)): getTaskVirtual(getTaskVirtual) {}

public:
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

	inline Task *getTask() {
		return getTaskVirtual(this);
	}
};

#endif /* BLOCKABLE_H_ */
