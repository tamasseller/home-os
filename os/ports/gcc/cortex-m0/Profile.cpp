/*
 * Profile.cpp
 *
 *  Created on: 2017.05.24.
 *      Author: tooma
 */

#include "Profile.h"

volatile uint32_t ProfileCortexM0::Timer::tick = 0;

void* volatile ProfileCortexM0::Task::newContext;
void** volatile ProfileCortexM0::Task::oldContext;
volatile bool ProfileCortexM0::CallGate::isAsyncActive = false;
void (*ProfileCortexM0::CallGate::asyncCallHandler)();

void ProfileCortexM0::Task::startFirst()
{

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
		"msr CONTROL, r0	\n"
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
		static void restoreMasterState() {
			asm volatile (
				"movs r0, #0		\n"
				"msr CONTROL, r0	\n"
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
	asm volatile (
		"mrs r0, psp			\n"
		"sub r0, r0, #32		\n"
		"stmia r0!, {r4-r7}		\n" /* Store remaining low regs */

		"str r0, [%[out], #0]	\n"

		"mov r4, r8				\n"
		"mov r5, r9				\n"
		"mov r6, r10			\n"
		"mov r7, r11			\n"
		"stmia r0!, {r4-r7}		\n" /* Store remaining high regs */

		"ldmia %[in]!, {r4-r7}	\n" /* Load high regs*/
		"mov r8, r4				\n"
		"mov r9, r5				\n"
		"mov r10, r6			\n"
		"mov r11, r7			\n"

		"msr psp, %[in]			\n" /* Save original top of stack */
		"sub %[in], %[in], #32	\n" /* Go back down for the low regs */
		"ldmia %[in]!, {r4-r7}	\n" /* Load low regs*/
		"bx lr					\n" /* Return */
			: :
			[in] "r" (Task::newContext),
			[out] "r" (Task::oldContext) :
	);
}

void SysTick_Handler()
{
	using Timer = ProfileCortexM0::Internals::Timer;
	using CallGate = ProfileCortexM0::Internals::CallGate;
	using Scb = ProfileCortexM0::Internals::Scb;

	while(true) {
		if(Scb::Syst::triggeredByCounter()) {
			Timer::tick++;
			continue;
		};

		if(CallGate::isAsyncActive) {
			CallGate::isAsyncActive = false;
			CallGate::asyncCallHandler();
			continue;
		}

		break;
	}
}
