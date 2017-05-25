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
	asm volatile ("MRS %0, psp\n"  : "=r" (psp));
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
		"bx r12\n" : : :
	);
}

__attribute__((naked))
void PendSV_Handler() {

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
			[in] "r" (ProfileCortexM0::Task::newContext),
			[out] "r" (ProfileCortexM0::Task::oldContext) :
	);
}

void SysTick_Handler()
{
	if(ProfileCortexM0::Scb::Syst::triggeredByCounter())
		ProfileCortexM0::Timer::tick++;
}
