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
class Scheduler<Args...>::BinarySemaphore: public SemaphoreLikeBlocker<BinarySemaphore>
{
	friend Scheduler<Args...>;
	bool open = true;

	virtual Blocker* getTakeable(Task*) override final {
		return open ? this : nullptr;
	}

	virtual uintptr_t acquire(Task*) override final {
		open = false;
		return true;
	}

	virtual bool release(uintptr_t arg) override final {
		if(!open) {
			if(!this->wakeOne()) {
				open = true;
			} else
				return true;
		}

		return false;
	}

public:
	void init(bool open) {
		this->open = open;
	}
};

#endif /* BINARYSEMAPHORE_H_ */
