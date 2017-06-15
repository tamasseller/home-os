/*
 * Atomic.h
 *
 *  Created on: 2017.06.11.
 *      Author: tooma
 */

#ifndef ATOMIC_H_
#define ATOMIC_H_

#include "Profile.h"

template<class Data>
class ProfileLinuxUm::Atomic {
	volatile Data data;
public:
	inline Atomic(): data(0) {}

	inline Atomic(const Data& value) {
		data = value;
	}

	inline operator Data() {
		return data;
	}

	template<class Op, class... Args>
	inline Data operator()(Op&& op, Args... args)
	{
		Data old, result;

		do {
			old = this->data;

			if(!op(old, result, args...))
				break;

		} while(!__sync_bool_compare_and_swap(&this->data, old, result));

		return old;
	}
};





#endif /* ATOMIC_H_ */
