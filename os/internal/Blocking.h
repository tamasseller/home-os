/*
 * Blocking.h
 *
 *  Created on: 2017.05.30.
 *      Author: tooma
 */

#ifndef BLOCKING_H_
#define BLOCKING_H_

#include "Scheduler.h"

#include "data/OrderedDoubleList.h"

template<class... Args>
class Scheduler<Args...>::Blockable {
	friend pet::DoubleList<Blockable>;

	Blockable *prev, *next;

	Task* (* const getTaskVirtual)(Blockable*);

protected:
	inline Blockable(Task* (*getTaskVirtual)(Blockable*)): getTaskVirtual(getTaskVirtual) {}

public:
	inline Task *getTask() {
		return getTaskVirtual(this);
	}
};

#endif /* BLOCKING_H_ */
