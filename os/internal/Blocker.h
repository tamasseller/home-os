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
	virtual void remove(Task*) = 0;
	virtual void waken(Task*, Waitable*) = 0;
	// TODO add handlePriorityChange method.
};

#endif /* BLOCKER_H_ */
