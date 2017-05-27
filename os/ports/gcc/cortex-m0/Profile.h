/*
 * CortexM0.h
 *
 *  Created on: 2017.05.24.
 *      Author: tooma
 */

#ifndef CORTEXM0_H_
#define CORTEXM0_H_

#include <stdint.h>

class ProfileCortexM0 {
	template<class, template<class > class> friend class Scheduler;

	class Task {
		friend void PendSV_Handler();
		static Task* volatile currentTask;
		static Task* volatile oldTask;

		template<class Type, void (Type::*entry)()> static void entryStub(Type* self);

		void* sp;

	public:
		template<class Type, void (Type::*entry)(), void exit()>
		inline void initialize(void* stack, uint32_t stackSize, Type* arg);

		void startFirst();
		void finishLast();

		void switchTo();
		static Task* getCurrent();
	};

	class Timer {
		friend void SysTick_Handler();

		static void (*tickHandler)();

		static volatile uint32_t tick;
	public:
		typedef uint32_t TickType;
		static inline TickType getTick();

		typedef uint32_t ClockType;
		static inline ClockType getClockCycle();

		static inline void setTickHandler(void (*tickHandler)());
	};

	class CallGate {
		friend void PendSV_Handler();
		static void (*asyncCallHandler)();

		static uintptr_t issueSvc(uintptr_t (*f)());
		static uintptr_t issueSvc(uintptr_t arg1, uintptr_t (*f)(uintptr_t));
		static uintptr_t issueSvc(uintptr_t arg1, uintptr_t arg2, uintptr_t (*f)(uintptr_t, uintptr_t));
		static uintptr_t issueSvc(uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t (*f)(uintptr_t, uintptr_t, uintptr_t));
		static uintptr_t issueSvc(uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4, uintptr_t (*f)(uintptr_t, uintptr_t, uintptr_t, uintptr_t));
		template<class ... Args> static inline uintptr_t callViaSvc(uintptr_t (f)(Args...), Args ... args);

	public:
		template<class ... T> static inline uintptr_t sync(T ... ops);
		static inline void async(void (*asyncCallHandler)());
	};

public:
	class Internals;

	static inline void init(uint32_t ticks);
};

class ProfileCortexM0::Internals: ProfileCortexM0 {
	friend void SysTick_Handler();
	friend void PendSV_Handler();
	friend ProfileCortexM0;

	class Scb {
		friend void SysTick_Handler();
		friend ProfileCortexM0;
		class Icsr {
			static constexpr auto reg = ((volatile uint32_t *) 0xe000ed04);
			static constexpr uint32_t PendSvSet = 1u << 28;
		public:
			static inline void triggerPendSV() {
				*reg = PendSvSet;
			}
		};

		class Syst {
			static constexpr auto ctrl = ((volatile uint32_t *) 0xe000e010);
			static constexpr auto load = ((volatile uint32_t *) 0xe000e014);
			static constexpr uint32_t SourceHclk  = 1u << 2;
			static constexpr uint32_t EnableIrq   = 1u << 1;
			static constexpr uint32_t EnableTimer = 1u << 0;

		public:

			static void init(uint32_t ticks) {
				*load = ticks-1;
				*ctrl |=  SourceHclk | EnableIrq | EnableTimer;
			}
		};
	};

	using ProfileCortexM0::Task;
	using ProfileCortexM0::Timer;
	using ProfileCortexM0::CallGate;
};

inline void ProfileCortexM0::init(uint32_t ticks) {
	Internals::Scb::Syst::init(ticks);

/*    SCB->SHP[_SHP_IDX(IRQn)] = (SCB->SHP[_SHP_IDX(IRQn)] & ~(0xFF << _BIT_SHIFT(IRQn))) |
        (((priority << (8 - __NVIC_PRIO_BITS)) & 0xFF) << _BIT_SHIFT(IRQn)); }*/

}

template<class ... Args>
inline uintptr_t ProfileCortexM0::CallGate::callViaSvc(uintptr_t (f)(Args...), Args ... args) {
	return issueSvc((uintptr_t)args..., f);
}
template<class ... T>
inline uintptr_t ProfileCortexM0::CallGate::sync(T ... ops) {
	return callViaSvc(ops...);
}

inline void ProfileCortexM0::CallGate::async(void (*asyncCallHandler)()) {
	CallGate::asyncCallHandler = asyncCallHandler;
	Internals::Scb::Icsr::triggerPendSV();
}

template<class Type, void (Type::*entry)()>
void ProfileCortexM0::Task::entryStub(Type* self) {
	(self->*entry)();
}

template<class Type, void (Type::*entry)(), void exit()>
inline void ProfileCortexM0::Task::initialize(void* stack, uint32_t stackSize, Type* arg) {
	uintptr_t* data = (uintptr_t*) stack + (stackSize / 4 - 1);

	void (*stub)(Type*) = &entryStub<Type, entry>;

	*data-- = 0x01000000;		// xpsr
	*data-- = (uintptr_t) stub;	// pc
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
	sp = data + 1;
	*data-- = 0xbaadf00d;		// r7
	*data-- = 0xbaadf00d;		// r6
	*data-- = 0xbaadf00d;		// r5
	*data-- = 0xbaadf00d;		// r4
}

inline void ProfileCortexM0::Task::switchTo() {
	oldTask = currentTask;
	currentTask = this;
	Internals::Scb::Icsr::triggerPendSV();
}

inline ProfileCortexM0::Task* ProfileCortexM0::Task::getCurrent() {
	return currentTask;
}

inline ProfileCortexM0::Timer::TickType ProfileCortexM0::Timer::getTick() {
	return tick;
}

inline ProfileCortexM0::Timer::ClockType ProfileCortexM0::Timer::getClockCycle() {
	return 0;
}

inline void ProfileCortexM0::Timer::setTickHandler(void (*tickHandler)()) {
	ProfileCortexM0::Timer::tickHandler = tickHandler;
}

#endif /* CORTEXM0_H_ */
