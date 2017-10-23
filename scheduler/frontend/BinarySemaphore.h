/*
 * BinarySemaphore.h
 *
 *  Created on: 2017.06.02.
 *      Author: tooma
 */

#ifndef BINARYSEMAPHORE_H_
#define BINARYSEMAPHORE_H_

#include "Scheduler.h"

template<class... Args>
class Scheduler<Args...>::BinarySemaphore: public Waitable
{
	friend Scheduler<Args...>;
	bool open = true;

	virtual bool wouldBlock() {
		return !open;
	}

	virtual void acquire()
	{
		open = false;
	}

	virtual void release(uintptr_t arg)
	{
		if(!open) {
			if(!Waitable::wakeOne())
				open = true;
		}
	}

public:
	void init(bool open) {
		this->open = open;
		Waitable::init();
	}
};

#endif /* BINARYSEMAPHORE_H_ */
