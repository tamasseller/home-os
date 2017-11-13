/*******************************************************************************
 *
 * Copyright (c) 2017 Seller Tamás. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *******************************************************************************/

#ifndef COUNTINGSEMAPHORE_H_
#define COUNTINGSEMAPHORE_H_

#include "Scheduler.h"

namespace home {

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

}

#endif /* COUNTINGSEMAPHORE_H_ */
