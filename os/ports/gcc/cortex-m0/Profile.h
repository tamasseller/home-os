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
	template<class, template<class> class> friend class Scheduler;

	class Task {
		friend void PendSV_Handler();
		static void* volatile newContext;
		static void** volatile oldContext;

		void* sp;

		template<class Type, void (Type::* entry)()> static void entryStub(Type* self);

	public:
		template<class Type, void (Type::* entry)(), void exit()>
		inline void initialize(void* stack, uint32_t stackSize, Type* arg);
		inline void switchTo(Task*);

		void startFirst();
		void finishLast();
	};

	class Timer {
		friend void SysTick_Handler();
		static volatile uint32_t tick;
	public:
		typedef uint32_t TickType;
		static inline TickType getTick();

		typedef uint32_t ClockType;
		static inline ClockType getClockCycle();
	};

	class CallGate {
		friend void SysTick_Handler();
		static volatile bool isAsyncActive;
		static void (*asyncCallHandler)();

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

		static inline void async();

		inline void setAsyncCallHandler(void (*asyncCallHandler)()) {
			CallGate::asyncCallHandler = asyncCallHandler;
		}
	};

public:
	class Internals;
};

class ProfileCortexM0::Internals: ProfileCortexM0 {
	friend void SysTick_Handler();
	friend void PendSV_Handler();
	friend ProfileCortexM0;

	class Scb {
		friend void SysTick_Handler();
		friend ProfileCortexM0;
		class Icsr {
			static constexpr auto reg = ((volatile uint32_t *)0xE000ED04);
			static constexpr uint32_t PendSvSet = 1u << 28;
			static constexpr uint32_t PendStSet = 1u << 26;

		public:
			static inline void triggerPendSV()
			{
				*reg = PendSvSet;
			}

			static inline void triggerSysTick()
			{
				*reg = PendStSet;
			}
		};

		class Syst {
			static constexpr auto reg = ((volatile uint32_t *)0xE000E010);
			static constexpr uint32_t Countflag = 1u << 16;
		public:
			static inline bool triggeredByCounter()
			{
				return *reg & Countflag;
			}
		};
	};

	using ProfileCortexM0::Task;
	using ProfileCortexM0::Timer;
	using ProfileCortexM0::CallGate;
};

template<class Type, void (Type::* entry)()>
void ProfileCortexM0::Task::entryStub(Type* self) {
	(self->*entry)();
}


inline void ProfileCortexM0::CallGate::async() {
	return Internals::Scb::Icsr::triggerSysTick();
}

template<class Type, void (Type::* entry)(), void exit()>
inline void ProfileCortexM0::Task::initialize(void* stack, uint32_t stackSize, Type* arg)
{
	uintptr_t* data = (uintptr_t*)stack + (stackSize / 4 - 1);

	void (*stub)(Type*) = &entryStub<Type, entry>;

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
	Internals::Scb::Icsr::triggerPendSV();
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
