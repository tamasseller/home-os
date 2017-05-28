/*
 * Task.h
 *
 *  Created on: 2017.05.27.
 *      Author: tooma
 */

#ifndef PROFILE_TASK_H_
#define PROFILE_TASK_H_

#include "Profile.h"

class ProfileCortexM0::Task {
	friend void PendSV_Handler();
	static Task* volatile currentTask;
	static Task* volatile oldTask;
	static void* suspendedPc;

	template<class Type, void (Type::*entry)()> static void entryStub(Type* self);

	void* sp;

	static constexpr auto frameSize = 16u * 4u;

	static inline void* &stackedPc();
public:
	template<class Type, void (Type::*entry)(), void exit()>
	inline void initialize(void* stack, uint32_t stackSize, Type* arg);

	void startFirst();
	void finishLast();

	inline void switchTo();
	inline static Task* getCurrent();

	static inline void suspendExecution();
};

///////////////////////////////////////////////////////////////////////////////

template<class Type, void (Type::*entry)()>
void ProfileCortexM0::Task::entryStub(Type* self) {
	(self->*entry)();
}

template<class Type, void (Type::*entry)(), void exit()>
inline void ProfileCortexM0::Task::initialize(void* stack, uint32_t stackSize, Type* arg) {
	uintptr_t* data = (uintptr_t*) stack + (stackSize / 4 - 1);

	// assert(stackSize >= frameSize);

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

inline void* &ProfileCortexM0::Task::stackedPc()
{
	void **psp;
	asm volatile ("mrs %0, psp\n"  : "=r" (psp));
	return psp[6];
}

inline void ProfileCortexM0::Task::switchTo()
{
	if(suspendedPc) {
		stackedPc() = suspendedPc;
		suspendedPc = nullptr;
	}

	oldTask = currentTask;
	currentTask = this;
	Internals::Scb::Icsr::triggerPendSV();
}

inline ProfileCortexM0::Task* ProfileCortexM0::Task::getCurrent()
{
	if(suspendedPc)
		return nullptr;

	return currentTask;
}

extern int asd;

inline void ProfileCortexM0::Task::suspendExecution() {
	struct Idler {
		__attribute__((naked))
		static void sleep() {
			while(true) {
				asd++;
				asm("wfi\n");
			}
		}
	};

	suspendedPc = stackedPc();
	stackedPc() = (void*)&Idler::sleep;
}

#endif /* PROFILE_TASK_H_ */
