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

#ifndef WAITABLEBLOCKER_H_
#define WAITABLEBLOCKER_H_

#include "Scheduler.h"

template<class... Args>
template<class Semaphore>
struct Scheduler<Args...>::WaitableBlocker
{
	inline void wait() {
		Scheduler<Args...>::blockOn(static_cast<Semaphore*>(this));
	}

	inline bool wait(uintptr_t timeout) {
		return Scheduler<Args...>::timedBlockOn(static_cast<Semaphore*>(this), timeout);
	}
};

#endif /* WAITABLEBLOCKER_H_ */
