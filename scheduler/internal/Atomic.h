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

#ifndef INTERNAL_ATOMIC_H_
#define INTERNAL_ATOMIC_H_

#include "Scheduler.h"

namespace home {

template<class... Args>
template<class Data>
struct Scheduler<Args...>::Atomic: Profile::template Atomic<Data>
{
	inline Atomic(): Profile::template Atomic<Data>(0) {}
	inline Atomic(Data value): Profile::template Atomic<Data>(value) {}

	Data increment() {
		return (*this)([](Data old, Data& result){
			result = static_cast<Data>(old + 1);
			return true;
		});
	}

	Data increment(Data amount) {
		return (*this)([](Data old, Data& result, Data amount){
			result = static_cast<Data>(old + amount);
			return true;
		}, amount);
	}

	Data decrement() {
		return (*this)([](Data old, Data& result){
			result = static_cast<Data>(old - 1);
			return true;
		});
	}

	Data reset() {
		return (*this)([](Data old, Data& result){
			result = 0;
			return true;
		});
	}

	bool compareAndSwap(Data expected, Data update)
	{
	    bool done = false;

		return expected == (*this)([&done, expected, update](Data old, Data& result){
			if(old == expected) {
			    done = true;
				result = update;
				return true;
			} else {
			    done = false;
			    return false;
			}
		});

		return done;
	}
};

}

#endif /* INTERNAL_ATOMIC_H_ */
