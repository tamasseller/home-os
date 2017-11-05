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
class Scheduler<Args...>::CountingSemaphore: public SemaphoreLikeBlocker<CountingSemaphore>, Registry<CountingSemaphore>::ObjectBase
{
	friend Scheduler<Args...>;
	uintptr_t counter;

	virtual Blocker* getTakeable(Task*) override final {
		return counter ? this : nullptr;
	}

	virtual uintptr_t acquire(Task*) override final {
		counter--;
		return true;
	}

	virtual bool release(uintptr_t arg) override final {
		bool ret = false;

		if(!counter) {
			while(arg && this->wakeOne())
				arg--;

			ret = true;
		}

		counter += arg;
		return ret;
	}

public:
	inline void init(uintptr_t count) {
		resetObject(this);
		counter = count;
		Registry<CountingSemaphore>::registerObject(this);
	}

	 ~CountingSemaphore() {
		Registry<CountingSemaphore>::unregisterObject(this);
	}
};

#endif /* COUNTINGSEMAPHORE_H_ */
