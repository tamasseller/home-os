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

#ifndef SEMAPHORELIKEBLOCKER_H_
#define SEMAPHORELIKEBLOCKER_H_

#include "Scheduler.h"

namespace home {

template<class... Args>
template<class Semaphore>
class Scheduler<Args...>::SemaphoreLikeBlocker:
		public SharedBlocker,
		public WaitableBlocker<Semaphore>,
		public AsyncBlocker<Semaphore>
{
	friend class Scheduler<Args...>;
	static constexpr uintptr_t timeoutReturnValue = 0;
	static constexpr uintptr_t blockedReturnValue = 1;

protected:
	inline bool wakeOne()
	{
		if(Blockable* blockable = this->waiters.lowest()) {
			blockable->wake(this);
			return true;
		}

		return false;
	}

	virtual void canceled(Blockable* ba, Blocker* be) final override {
		this->waiters.remove(ba);
	}

	virtual void timedOut(Blockable* ba) final override {
		this->waiters.remove(ba);
		Profile::injectReturnValue(ba->getTask(), Semaphore::timeoutReturnValue);
	}
};

}

#endif /* SEMAPHORELIKEBLOCKER_H_ */
