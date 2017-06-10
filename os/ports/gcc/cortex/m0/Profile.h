/*
 * CortexM0.h
 *
 *  Created on: 2017.05.24.
 *      Author: tooma
 */

#ifndef CORTEXM0_H_
#define CORTEXM0_H_

#include <stdint.h>

#include "../common/Atomic.h"

class ProfileCortexM0 {
	template<class Value> inline static Value loadExclusive(volatile Value*);
	template<class Value> inline static bool storeExclusive(volatile Value*, Value);
	inline static void clearExclusive();

	static volatile bool exclusiveMonitor;
public:
	class Task;
	class Timer;
	class CallGate;

	template<class Data>
	using Atomic = CortexCommon::Atomic<Data, &loadExclusive, &storeExclusive, &clearExclusive>;

	class Internals;

	static inline void init(uint32_t ticks, uint8_t systickPrio=0xc0);
};

#include "Internals.h"
#include "Task.h"
#include "Timer.h"
#include "CallGate.h"

///////////////////////////////////////////////////////////////////////////////

inline void ProfileCortexM0::init(uint32_t ticks, uint8_t systickPrio) {
	Internals::Scb::Syst::init(ticks);
	Internals::Scb::Shpr::init(0xc0, systickPrio, 0xc0);
	Timer::tick = 0;
}

template<class Value>
inline Value ProfileCortexM0::loadExclusive(volatile Value* addr)
{
	exclusiveMonitor = true;
	return *addr;
}

inline void ProfileCortexM0::clearExclusive()
{
	exclusiveMonitor = false;
}

template<class Value>
inline bool ProfileCortexM0::storeExclusive(volatile Value* addr, Value in)
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
			: [ret] "=&r" (ret)
			: [mon] "r"   (&exclusiveMonitor),
			  [tmp] "r"   (temp),
			  [adr] "r"   (addr),
			  [in]  "r"   (in) :
		);

	return ret;
}

#endif /* CORTEXM0_H_ */
