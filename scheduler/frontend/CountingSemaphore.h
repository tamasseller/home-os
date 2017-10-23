/*
 * CountingSemaphore.h
 *
 *  Created on: 2017.06.11.
 *      Author: tooma
 */

#ifndef COUNTINGSEMAPHORE_H_
#define COUNTINGSEMAPHORE_H_

#include "Scheduler.h"

template<class... Args>
class Scheduler<Args...>::CountingSemaphore: public Waitable
{
	friend Scheduler<Args...>;
	uintptr_t counter;

	virtual bool wouldBlock() {
		return !counter;
	}

	virtual void acquire()
	{
		counter--;
	}

	virtual void release(uintptr_t arg)
	{
		if(!counter) {
			while(arg && Waitable::wakeOne())
				arg--;

			counter += arg;
		}
	}

public:
	void init(uintptr_t count) {
		counter = count;
		Waitable::init();
	}
};

#endif /* COUNTINGSEMAPHORE_H_ */
