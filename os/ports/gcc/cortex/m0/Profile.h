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
#include "../common/Scb.h"

class ProfileCortexM0 {
	template<class Value> inline static Value loadExclusive(volatile Value*);
	template<class Value> inline static bool storeExclusive(volatile Value*, Value);
	inline static void clearExclusive();

	static volatile bool exclusiveMonitor;

	static inline void sysWrite0(const char* msg);
public:
	class Task;
	class Timer;
	class CallGate;

	template<class Data>
	using Atomic = CortexCommon::Atomic<Data, &loadExclusive, &storeExclusive, &clearExclusive>;

	static inline void init(uint32_t ticks, uint8_t systickPrio=0xc0);
	static inline void setSyscallMapper(uint32_t ticks, uint8_t systickPrio=0xc0);

	static inline void fatalError(const char* msg);
};

#include "Task.h"
#include "Timer.h"
#include "CallGate.h"

///////////////////////////////////////////////////////////////////////////////

inline void ProfileCortexM0::init(uint32_t ticks, uint8_t systickPrio) {
	CortexCommon::Scb::Syst::init(ticks);
	CortexCommon::Scb::Shpr::init(0xc0, systickPrio, 0xc0);
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

inline void ProfileCortexM0::sysWrite0(const char* msg)
{
	/*
	 * SYS_WRITE0 (0x04)
	 *
	 * 		Writes a null-terminated string to the debug channel.
	 * 		On entry, R1 contains a pointer to the first byte of the string.
	 */

	register uintptr_t opCode asm("r0") = 4;
	register uintptr_t str asm("r1") = (uintptr_t)msg;
 	asm volatile("bkpt #0xab" : "=r"(opCode), "=r"(str) : "r" (opCode), "r" (str) : "memory");
}

inline void ProfileCortexM0::fatalError(const char* msg)
{
	sysWrite0("Fatal error: ");
	sysWrite0(msg);
	sysWrite0("!\n");
}

#endif /* CORTEXM0_H_ */
