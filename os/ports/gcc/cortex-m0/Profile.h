/*
 * CortexM0.h
 *
 *  Created on: 2017.05.24.
 *      Author: tooma
 */

#ifndef CORTEXM0_H_
#define CORTEXM0_H_

#include <stdint.h>

class ProfileCortexM0
{
	template<class, class> friend class Scheduler;
	friend void SysTick_Handler();
	friend void PendSV_Handler();

	class Task {
		friend void PendSV_Handler();
		void* sp;

		static void* volatile newContext;
		static void** volatile oldContext;

	public:
		template<void entry(void*), void exit()>
		inline void initialize(void* stack, uint32_t stackSize, void* arg);
		inline void switchTo(Task*);

		void startFirst();
	};

	class Timer {
		static volatile uint32_t tick;
		friend void SysTick_Handler();
	public:
		typedef uint32_t TickType;
		static inline TickType getTick();

		typedef uint32_t ClockType;
		static inline ClockType getClockCycle();
	};

	class CallGate {
		static uintptr_t issueSvc(uintptr_t (*f)());
		static uintptr_t issueSvc(uintptr_t arg1, uintptr_t (*f)(uintptr_t));
		static uintptr_t issueSvc(uintptr_t arg1, uintptr_t arg2, uintptr_t (*f)(uintptr_t, uintptr_t));
		static uintptr_t issueSvc(uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t (*f)(uintptr_t, uintptr_t, uintptr_t));
		static uintptr_t issueSvc(uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4, uintptr_t (*f)(uintptr_t, uintptr_t, uintptr_t, uintptr_t));

		template<class... Args>
		static inline uintptr_t call(uintptr_t (f)(Args...), Args... args) {
			return issueSvc(args..., f);
		}
	public:
		template<class... T>
		static inline uintptr_t issue(T... ops) {
			return call(ops...);
		}
	};
};

template<void entry(void*), void exit()>
inline void ProfileCortexM0::Task::initialize(void* stack, uint32_t stackSize, void* arg)
{
	uintptr_t* data = (uintptr_t*)stack + (stackSize / 4 - 1);

	*data-- = 0x01000000;		// xpsr
	*data-- = (uintptr_t)entry;	// pc
	*data-- = (uintptr_t)exit;	// lr
	*data-- = 0xbaadf00d;		// r12
	*data-- = 0xbaadf00d;		// r3
	*data-- = 0xbaadf00d;		// r2
	*data-- = 0xbaadf00d;		// r1
	*data-- = (uintptr_t)arg;	// r0
	*data-- = 0xbaadf00d;		// r11
	*data-- = 0xbaadf00d;		// r10
	*data-- = 0xbaadf00d;		// r9
	*data-- = 0xbaadf00d;		// r8
	sp = data+1;
	*data-- = 0xbaadf00d;		// r7
	*data-- = 0xbaadf00d;		// r6
	*data-- = 0xbaadf00d;		// r5
	*data-- = 0xbaadf00d;		// r4
}

inline void ProfileCortexM0::Task::switchTo(Task* replacement) {
	oldContext = &sp;
	newContext = replacement->sp;
	*((volatile uint32_t *)0xe000ed04) = 0x10000000;
}

inline ProfileCortexM0::Timer::TickType ProfileCortexM0::Timer::getTick()
{
	return tick;
}

inline ProfileCortexM0::Timer::ClockType ProfileCortexM0::Timer::getClockCycle()
{
	return 0;
}

#endif /* CORTEXM0_H_ */
