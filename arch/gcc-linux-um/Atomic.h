/*
 * Atomic.h
 *
 *  Created on: 2018.02.28.
 *      Author: tooma
 */

#ifndef ATOMIC_H_
#define ATOMIC_H_

namespace home {

template<class Data>
class Atomic {
	volatile Data data;
public:
	inline Atomic(): data(0) {}

	inline Atomic(Data value) {
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

}

#endif /* ATOMIC_H_ */
