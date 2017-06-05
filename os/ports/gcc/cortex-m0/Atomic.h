/*
 * Atomic.h
 *
 *  Created on: 2017.05.27.
 *      Author: tooma
 */

#ifndef ATOMIC_H_
#define ATOMIC_H_

template<class Value>
class ProfileCortexM0::Atomic
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
		Value old;

		asm volatile (
			"cpsid i\n"
			"ldr %[old], [%[data]]\n"
				: [old]"=l"(old)
				: [data]"l"(&this->data)
				: /* No clobbers */
		);

		Value result;
		if(op(old, result, args...))
			data = result;

		asm volatile ("cpsie i\n");
		return old;
	}
};

#endif /* ATOMIC_H_ */
