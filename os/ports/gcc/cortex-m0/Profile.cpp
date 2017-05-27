/*
 * Profile.cpp
 *
 *  Created on: 2017.05.24.
 *      Author: tooma
 */

#include "Profile.h"

volatile uint32_t ProfileCortexM0::Timer::tick = 0;

ProfileCortexM0::Task* volatile ProfileCortexM0::Task::currentTask;
ProfileCortexM0::Task* volatile ProfileCortexM0::Task::oldTask;

void (*ProfileCortexM0::CallGate::asyncCallHandler)();
void (*ProfileCortexM0::Timer::tickHandler)();

__attribute__((naked))
void ProfileCortexM0::Task::startFirst()
{
	currentTask = this;
	asm volatile (
		"push {r4-r7, lr}	\n"
		"mov r4, r8			\n"
		"mov r5, r9			\n"
		"mov r6, r10		\n"
		"mov r7, r11		\n"
		"push {r4-r7}		\n" : : : "r4", "r5", "r6", "r7"
	);

	asm volatile (
		"msr psp, %0		\n"
		"movs r0, #2		\n"
		"msr control, r0	\n"
		"isb				\n"
		"pop {r0-r5}		\n"
		"mov lr, r5			\n"
		"cpsie i			\n"
		"pop {pc}			\n" : : "r" ((uint32_t*)sp + 4) :
	);
}

void ProfileCortexM0::Task::finishLast()
{
	struct ReturnTaskStub {
		__attribute__((naked))
		static void restoreMasterState() {
			asm volatile (
				"movs r0, #0		\n"
				"msr control, r0	\n"
				"isb				\n"
				"pop {r4-r7}		\n"
				"mov r8, r4			\n"
				"mov r9, r5			\n"
				"mov r10, r6		\n"
				"mov r11, r7		\n"
				"pop {r4-r7, pc}	\n" : : :
			);
		}
	};

	void **psp;
	asm volatile ("mrs %0, psp\n"  : "=r" (psp));
	psp[6] = (void*)&ReturnTaskStub::restoreMasterState;
	currentTask = nullptr;
}

__attribute__((naked))
uintptr_t ProfileCortexM0::CallGate::issueSvc(uintptr_t (*f)())
{
	asm volatile (
		"mov r12, r0	\n"
		"svc 0			\n"
		"bx lr			\n" : : :
	);
}

__attribute__((naked))
uintptr_t ProfileCortexM0::CallGate::issueSvc(uintptr_t arg1, uintptr_t (*f)(uintptr_t))
{
	asm volatile (
		"mov r12, r1	\n"
		"svc 0			\n"
		"bx lr			\n" : : :
	);
}

__attribute__((naked))
uintptr_t ProfileCortexM0::CallGate::issueSvc(uintptr_t arg1, uintptr_t arg2, uintptr_t (*f)(uintptr_t, uintptr_t))
{
	asm volatile (
		"mov r12, r2	\n"
		"svc 0			\n"
		"bx lr			\n" : : :
	);
}

__attribute__((naked))
uintptr_t ProfileCortexM0::CallGate::issueSvc(uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t (*f)(uintptr_t, uintptr_t, uintptr_t))
{
	asm volatile (
		"mov r12, r3	\n"
		"svc 0			\n"
		"bx lr			\n" : : :
	);
}

__attribute__((naked))
uintptr_t ProfileCortexM0::CallGate::issueSvc(uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4, uintptr_t (*f)(uintptr_t, uintptr_t, uintptr_t, uintptr_t))
{
	asm volatile (
		"push {r4}			\n"
		"ldr r4, [sp, #4]	\n"
		"mov r4, r12		\n"
		"svc 0				\n"
		"pop {r4}			\n"
		"bx lr				\n" : : : "r4"
	);
}

__attribute__((naked))
void SVC_Handler() {
	asm volatile (
		"push {r4, lr}		\n"
		"mrs r4, psp		\n"
		"ldmia r4, {r0-r4}	\n" // Load stored regs (r0, r1, r2, r3, r12)
		"blx r4				\n"	// Call the one that is stored in the caller r12
		"mrs r1, psp		\n" // Save r0 to the caller frame
		"str r0, [r1, #0]	\n"
		"pop {r4, pc}		\n" : : :
	);
}

__attribute__((naked))
void PendSV_Handler() {
	using Task = ProfileCortexM0::Internals::Task;
	using CallGate = ProfileCortexM0::Internals::CallGate;

/*	if(CallGate::isAsyncActive) {
		CallGate::isAsyncActive = false;
		CallGate::asyncCallHandler();
	}*/

	asm volatile (
		".thumb					\n"
		".syntax unified		\n"

		"ldr r0, PendSV_async 	\n" // r0 = (void(*)()*) &CallGate::asyncCallHandler
		"ldr r0, [r0]			\n" // r0 = (void(*)()) CallGate::asyncCallHandler
		"cmp r0, #0				\n"	// if(CallGate::asyncCallHandler) {
		"beq PendSV_noAsync		\n"	//
		"push {lr}				\n"
		"blx r0					\n"	// CallGate::asyncCallHandler();
		"pop {r0}				\n"
		"mov lr, r0				\n"
		"PendSV_noAsync:		\n"	// }


		"ldr r0, PendSV_old		\n" // r0 = (Task**) &Task::oldTask
		"ldr r1, [r0]			\n" // r1 = (Task*) Task::oldTask

		"cmp r1, #0				\n"	// if(!Task::oldTask)
		"beq PendSV_done		\n"	// 		goto done;

		"mrs r2, psp			\n" // r2 = psp
		"subs r2, r2, #32		\n" // r2 -= 8 * 4					// make room for additional registers
		"stmia r2!, {r4-r7}		\n" // *r2++ = {r4, r5, r6, r7} 	// save low regs

		"str r2, [r1]			\n"	// Task::oldTask->sp = r2		// save mid-frame stack pointer
		"movs r3, #0			\n"
		"str r3, [r0]			\n" // *r0 = nullptr				// zero out Task::oldTask

		"mov r4, r8				\n"
		"mov r5, r9				\n"
		"mov r6, r10			\n"
		"mov r7, r11			\n"
		"stmia r2!, {r4-r7}		\n" // *r2++ = {r8, r9, r10, r11} 	// store remaining high regs

		"ldr r0, PendSV_new		\n"	// r0 = (Task**) &Task::oldTask
		"ldr r0, [r0]			\n" // r0 = Task::oldTask
		"ldr r0, [r0]			\n" // r0 = Task::oldTask->sp

		"ldmia r0!, {r4-r7}		\n" /* Load high regs*/
		"mov r8, r4				\n"
		"mov r9, r5				\n"
		"mov r10, r6			\n"
		"mov r11, r7			\n"

		"msr psp, r0			\n" /* Save original top of stack */
		"subs r0, r0, #32		\n" /* Go back down for the low regs */
		"ldmia r0!, {r4-r7}		\n" /* Load low regs*/
		"PendSV_done:			\n"
		"bx lr					\n" /* Return */
		".align 4				\n"
		"PendSV_old:			\n"
		"	.word %c0			\n"
		"PendSV_new:			\n"
		"	.word %c1			\n"
		"PendSV_async:			\n"
		"	.word %c2			\n"
			:
			: "i" (&Task::oldTask),
			  "i" (&Task::currentTask),
			  "i" (&CallGate::asyncCallHandler)
			: "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12"
	);
}

void SysTick_Handler()
{
	using Timer = ProfileCortexM0::Internals::Timer;
	Timer::tick++;
	Timer::tickHandler();
}
