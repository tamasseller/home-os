/*
 * Interrupts.cpp
 *
 *  Created on: 2017.05.24.
 *      Author: tooma
 */

#include "Profile.h"

__attribute__((naked))
void SVC_Handler() {
	asm volatile (
		"push {lr}\n"
		"blx r12\n"
		"pop {pc}\n" : : :
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
			: /* No outputs */
			: [in] "r" (ProfileCortexM0::Task::newContext), [out] "r" (ProfileCortexM0::Task::oldContext)
			: /* No clobbers */
	);
}

void SysTick_Handler()
{
	ProfileCortexM0::Timer::tick++;
}
