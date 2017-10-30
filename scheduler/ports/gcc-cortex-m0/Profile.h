/*
 * Profile.h
 *
 *  Created on: 2017.05.24.
 *      Author: tooma
 */

#ifndef PROFILE_H_
#define PROFILE_H_

#include <stdint.h>

#include "../gcc-cortex-common/Atomic.h"
#include "../gcc-cortex-common/Scb.h"
#include "../gcc-cortex-common/Svc.h"

class ProfileCortexM0 {
	friend void PendSV_Handler();
	friend void SVC_Handler();
	friend void SysTick_Handler();

	template<class Value> inline static bool storeExclusive(volatile Value*, Value);

	template<class Value>
	static inline Value loadExclusive(volatile Value* addr)
	{
		exclusiveMonitor = true;
		Value ret = *addr;
		asm volatile("":::"memory");
		return ret;
	}

	static inline void clearExclusive()
	{
		exclusiveMonitor = false;
		asm volatile("":::"memory");
	}

public:
	class Task {
		friend ProfileCortexM0;
		void* sp;
	};

	template<class Data>
	using Atomic = CortexCommon::Atomic<Data, &loadExclusive, &storeExclusive, &clearExclusive>;

	typedef uint32_t TickType;
	typedef uint32_t ClockType;

private:
	static void (* volatile asyncCallHandler)();
	static void* (* volatile syncCallMapper)(uintptr_t);

	static volatile uint32_t tick;
	static void (*tickHandler)();

	static volatile bool exclusiveMonitor;

	static Task* volatile currentTask;
	static Task* volatile oldTask;
	static void* suspendedPc;

	static inline void sysWrite0(const char* msg);

	static inline void* &irqEntryStackedPc() {
		void **psp;
		asm volatile ("mrs %0, psp\n"  : "=r" (psp));
		return psp[6];
	}

public:
	static inline void init(uint32_t ticks, uint8_t systickPrio=0xc0) {
		CortexCommon::Scb::Syst::init(ticks);
		CortexCommon::Scb::Shpr::init(0xc0, systickPrio, 0xc0);
		tick = 0;
	}


	static inline void setSyscallMapper(void* (*mapper)(uintptr_t)) {
		syncCallMapper = mapper;
	}

	template<class ... T>
	static inline uintptr_t sync(T ... ops) {
        return CortexCommon::DirectSvc::issueSvc((uintptr_t)ops...);
	}

	static inline void async(void (*handler)()) {
		asyncCallHandler = handler;
		CortexCommon::Scb::Icsr::triggerPendSV();
	}

	static inline TickType getTick() {
		return tick;
	}

	static inline ClockType getClockCycle() {
		return 0;
	}

	static inline void setTickHandler(void (*handler)()) {
		tickHandler = handler;
	}

	static void startFirst(Task* task);
	static void finishLast();

	static inline void initialize(Task*, void (*)(void*), void (*)(), void*, uint32_t, void*);
	static inline void switchToAsync(Task* task)
	{
		if(suspendedPc) {
			irqEntryStackedPc() = suspendedPc;
			suspendedPc = nullptr;
		}

		if(!oldTask)
			oldTask = currentTask;

		currentTask = task;
	}

	static inline void switchToSync(Task* task)
	{
		switchToAsync(task);
		CortexCommon::Scb::Icsr::triggerPendSV();
	}

	static inline void injectReturnValue(Task* task, uintptr_t ret)
	{
		if(suspendedPc && task == currentTask) {
			uintptr_t* frame;
			asm volatile ("mrs %0, psp\n"  : "=r" (frame));
			frame[0] = ret;
		} else {
			uintptr_t* frame = reinterpret_cast<uintptr_t*>(task->sp);
			frame[4] = ret;
		}
	}

	static inline Task* getCurrent()
	{
		if(suspendedPc)
			return nullptr;

		return currentTask;
	}

	static inline void suspendExecution() {
		struct Idler {
			__attribute__((naked))
			static void sleep() {
				asm("sleep_until_interrupted:		\n"
					"	wfi							\n"
					"	b sleep_until_interrupted	\n");
			}
		};

		suspendedPc = irqEntryStackedPc();
		irqEntryStackedPc() = (void*)&Idler::sleep;
	}

	static inline void fatalError(const char* msg) {
		sysWrite0("Fatal error: ");
		sysWrite0(msg);
		sysWrite0("!\n");
	}
};

///////////////////////////////////////////////////////////////////////////////

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
			: [ret] "=&l" (ret)
			: [mon] "l"   (&exclusiveMonitor),
			  [tmp] "l"   (temp),
			  [adr] "l"   (addr),
			  [in]  "l"   (in) : "memory"
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

inline void ProfileCortexM0::initialize(Task* task, void (*entry)(void*), void (*exit)(),
		void* stack, uint32_t stackSize, void* arg)
{
	uintptr_t* data = (uintptr_t*) stack + (stackSize / 4 - 1);

	// assert(stackSize >= frameSize);

	*data-- = 0x01000000;		// xpsr (with thumb bit set)
	*data-- = (uintptr_t) entry;// pc
	*data-- = (uintptr_t) exit;	// lr
	*data-- = 0xbaadf00d;		// r12
	*data-- = 0xbaadf00d;		// r3
	*data-- = 0xbaadf00d;		// r2
	*data-- = 0xbaadf00d;		// r1
	*data-- = (uintptr_t) arg;	// r0
	*data-- = 0xbaadf00d;		// r11
	*data-- = 0xbaadf00d;		// r10
	*data-- = 0xbaadf00d;		// r9
	*data-- = 0xbaadf00d;		// r8
	task->sp = data + 1;
	*data-- = 0xbaadf00d;		// r7
	*data-- = 0xbaadf00d;		// r6
	*data-- = 0xbaadf00d;		// r5
	*data-- = 0xbaadf00d;		// r4
}

#endif /* PROFILE_H_ */
