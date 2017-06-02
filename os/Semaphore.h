/*
 * Semaphore.h
 *
 *  Created on: 2017.06.02.
 *      Author: tooma
 */

#ifndef SEMAPHORE_H_
#define SEMAPHORE_H_

#include "Scheduler.h"

template<class... Args>
class Scheduler<Args...>::BinarySemaphore: public Scheduler<Args...>::Waitable
{
	friend Scheduler<Args...>;
	bool open = true;

	virtual bool acquire()
	{
		if(open) {
			open = false;
			return true;
		}

		return false;
	}

	virtual void release(typename Waitable::WakeSession& session, uintptr_t arg)
	{
		if(!open) {
			if(!Waitable::wakeOne(session))
				open = true;
		}
	}

public:
	void init(bool open) {
		this->open = open;
		Waitable::init();
	}
};

#endif /* SEMAPHORE_H_ */
