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
	friend ProfileCortexM0;
	friend void PendSV_Handler();
	static Task* volatile currentTask;
	static Task* volatile oldTask;
	static void* suspendedPc;

	void* sp;

	static inline void* &irqEntryStackedPc()
	{
		void **psp;
		asm volatile ("mrs %0, psp\n"  : "=r" (psp));
		return psp[6];
	}

public:
	void startFirst();
	void finishLast();

	inline void initialize(void (*entry)(void*), void (*exit)(),
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
		sp = data + 1;
		*data-- = 0xbaadf00d;		// r7
		*data-- = 0xbaadf00d;		// r6
		*data-- = 0xbaadf00d;		// r5
		*data-- = 0xbaadf00d;		// r4
	}

	inline void switchToAsync()
	{
		if(suspendedPc) {
			irqEntryStackedPc() = suspendedPc;
			suspendedPc = nullptr;
		}

		if(!oldTask)
			oldTask = currentTask;

		currentTask = this;
	}

	inline void switchToSync()
	{
		switchToAsync();
		CortexCommon::Scb::Icsr::triggerPendSV();
	}

	inline void injectReturnValue(uintptr_t ret)
	{
		if(suspendedPc && this == currentTask) {
			uintptr_t* frame;
			asm volatile ("mrs %0, psp\n"  : "=r" (frame));
			frame[0] = ret;
		} else {
			uintptr_t* frame = reinterpret_cast<uintptr_t*>(sp);
			frame[4] = ret;
		}
	}

	static inline ProfileCortexM0::Task* getCurrent()
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
};


#endif /* PROFILE_TASK_H_ */
