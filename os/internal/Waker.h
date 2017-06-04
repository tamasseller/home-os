/*
 * Waker.h
 *
 *  Created on: 2017.06.02.
 *      Author: tooma
 */

#ifndef WAKER_H_
#define WAKER_H_

#include "Scheduler.h"

template<class... Args>
class Scheduler<Args...>::Waker {
	friend Scheduler<Args...>;
	virtual void remove(Task*) = 0;
	virtual void waken(Task*) = 0;
};


#endif /* WAKER_H_ */
