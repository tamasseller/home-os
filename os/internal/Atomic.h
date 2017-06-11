/*
 * Atomic.h
 *
 *  Created on: 2017.06.10.
 *      Author: tooma
 */

#ifndef INTERNAL_ATOMIC_H_
#define INTERNAL_ATOMIC_H_

#include "Scheduler.h"

template<class... Args>
template<class Data>
struct Scheduler<Args...>::Atomic: Profile::template Atomic<Data>
{
	inline Atomic(): Profile::template Atomic<Data>(0) {}
	inline Atomic(const Data& value): Profile::template Atomic<Data>(value) {}

	Data increment() {
		return (*this)([](Data old, Data& result){
			result = old + 1;
			return true;
		});
	}

	Data decrement() {
		return (*this)([](Data old, Data& result){
			result = old - 1;
			return true;
		});
	}

	Data reset() {
		return (*this)([](Data old, Data& result){
			result = 0;
			return true;
		});
	}
};

#endif /* INTERNAL_ATOMIC_H_ */
