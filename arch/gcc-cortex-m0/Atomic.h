/*
 * Atomic.h
 *
 *  Created on: 2018.02.28.
 *      Author: tooma
 */

#ifndef ATOMIC_H_
#define ATOMIC_H_

#include "AtomicCommon.h"

namespace home {

namespace detail {

extern volatile bool exclusiveMonitor;

template<class Value>
inline bool storeExclusive(volatile Value* addr, Value in)
{
	static_assert(sizeof(Value) == 4, "unimplemented atomic access width");
	uintptr_t temp = 0;
	bool ret;

	asm volatile(
		"	.thumb					\n"
		"	.syntax unified			\n"
		"	cpsid i					\n"	// Atomically:
		"	ldrb %[ret], [%[mon]]	\n"	// 		1. load the monitor.
		"	cmp %[ret], #0			\n" //  	2. check if contended.
		"	beq 1f					\n" //	 	3. skip next if it is.
		"	str %[in], [%[adr]]		\n" // 		4. store the new value.
		"1: strb %[tmp], [%[mon]]	\n" // 		5. reset monitor.
		"	cpsie i\n"
			: [ret] "=&l" (ret)
			: [mon] "l"   (&exclusiveMonitor),
			  [tmp] "l"   (temp),
			  [adr] "l"   (addr),
			  [in]  "l"   (in) : "memory"
		);

	return ret;
}

template<class Value>
inline Value loadExclusive(volatile Value* addr)
{
	exclusiveMonitor = true;
	Value ret = *addr;
	asm volatile("":::"memory");
	return ret;
}

inline void clearExclusive()
{
	exclusiveMonitor = false;
	asm volatile("":::"memory");
}

}

template<class Data>
struct Atomic: CortexCommon::AtomicCommon<
	Data,
	&detail::loadExclusive,
	&detail::storeExclusive,
	&detail::clearExclusive>
{
	inline Atomic(): Atomic::AtomicCommon(0) {}
	inline Atomic(Data data): Atomic::AtomicCommon(data) {}
};

}

#endif /* ATOMIC_H_ */
