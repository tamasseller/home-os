/*
 * CortexM0.h
 *
 *  Created on: 2017.05.24.
 *      Author: tooma
 */

#ifndef CORTEXM0_H_
#define CORTEXM0_H_

#include <stdint.h>

namespace DetailsForProfileCortexM0 {

	inline void triggerPendSV()
	{
		static auto &ScbIcsr = *((volatile uint32_t *)0xE000ED04);
		static constexpr uint32_t ScbIcsrPendsvSet = 0x10000000;
		ScbIcsr = ScbIcsrPendsvSet;
	}
}

class ProfileCortexM0
{
	inline void triggerPendSV();

	template<class, class> friend class Scheduler;
	friend void SysTick_Handler();
	friend void PendSV_Handler();

	class Task {
		friend void PendSV_Handler();
		void* sp;

		static void* volatile newContext;
		static void** volatile oldContext;

		template<class Type, void (Type::* entry)()>
		struct Entry {
			static void stub(Type* self) {
				(self->*entry)();
			}
		};

	public:
		template<class Type, void (Type::* entry)(), void exit()>
		inline void initialize(void* stack, uint32_t stackSize, Type* arg);
		inline void switchTo(Task*);

		void startFirst();
		void finishLast();
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
		static inline uintptr_t callViaSvc(uintptr_t (f)(Args...), Args... args) {
			return issueSvc((uintptr_t)args..., f);
		}
	public:
		template<class... T>
		static inline uintptr_t sync(T... ops) {
			return callViaSvc(ops...);
		}

		static inline void async() {
			return DetailsForProfileCortexM0::triggerPendSV();
		}

	};
};

template<class Type, void (Type::* entry)(), void exit()>
inline void ProfileCortexM0::Task::initialize(void* stack, uint32_t stackSize, Type* arg)
{
	uintptr_t* data = (uintptr_t*)stack + (stackSize / 4 - 1);

	void (*stub)(Type*) = &Entry<Type, entry>::stub;

	*data-- = 0x01000000;		// xpsr
	*data-- = (uintptr_t)stub;	// pc
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
	DetailsForProfileCortexM0::triggerPendSV();
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
