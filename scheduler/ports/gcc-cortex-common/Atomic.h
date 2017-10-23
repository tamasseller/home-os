/*
 * Atomic.h
 *
 *  Created on: 2017.05.27.
 *      Author: tooma
 */

#ifndef ATOMIC_H_
#define ATOMIC_H_

namespace CortexCommon {

template<class Value, Value (*ldrex)(volatile Value*), bool (*strex)(volatile Value*, Value), void (*clrex)()>
class Atomic
{
	volatile Value data;
public:
	inline Atomic(): data(0) {}

	inline Atomic(const Value& value) {
		data = value;
	}

	inline operator Value() {
		return data;
	}

	template<class Op, class... Args>
	inline Value operator()(Op&& op, Args... args)
	{
		Value old, result;
		do {
			old = ldrex(&this->data);

			if(!op(old, result, args...))
				clrex();

		} while(!strex(&this->data, result));

		return old;
	}
};

}

#endif /* ATOMIC_H_ */
