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

#ifndef BINARYSEMAPHORE_H_
#define BINARYSEMAPHORE_H_

#include "Scheduler.h"

namespace home {

template<class... Args>
class Scheduler<Args...>::BinarySemaphore: public SemaphoreLikeBlocker<BinarySemaphore>, Registry<BinarySemaphore>::ObjectBase
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
	inline void init(bool open) {
		resetObject(this);
		this->open = open;
		Registry<BinarySemaphore>::registerObject(this);
	}

	inline ~BinarySemaphore() {
		Registry<BinarySemaphore>::unregisterObject(this);
	}
};

}

#endif /* BINARYSEMAPHORE_H_ */
