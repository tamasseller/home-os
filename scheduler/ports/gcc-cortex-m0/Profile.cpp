/*******************************************************************************
 *
 * Copyright (c) 2017 Seller Tamás. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *******************************************************************************/

#include "Profile.h"

ProfileCortexM0::Task* volatile ProfileCortexM0::currentTask;
ProfileCortexM0::Task* volatile ProfileCortexM0::oldTask;
void* ProfileCortexM0::suspendedPc;
void* ProfileCortexM0::mspAtStart;

volatile bool ProfileCortexM0::exclusiveMonitor = false;

volatile uint32_t ProfileCortexM0::tick = 0;

void (*ProfileCortexM0::tickHandler)();
void (* volatile ProfileCortexM0::asyncCallHandler)();
void* (* volatile ProfileCortexM0::syncCallMapper)(uintptr_t);

__attribute__((naked))
const char* ProfileCortexM0::startFirst(Task* task)
{
	currentTask = task;

	asm volatile (
		"cpsid i			\n" // Only the systick, SVC and PendSV interrupts are needed to be disabled,
								// but on the Cortex-M0 the system interrupts can not be masked selectively.
		"push {r4-r7, lr}	\n" // ABI: r0-r3 and r12 can be destroyed during a procedure call.
		"mov r4, r8			\n" // r4-r11 and lr are stored here in a platform-convenient order.
		"mov r5, r9			\n" // pc will be restored by loading lr (i.e. returning)
		"mov r6, r10		\n" // sp is saved implicitly by keeping it in MSP, and using
		"mov r7, r11		\n" // PSP for the threads (interrupts will restore MSP on exit).
		"push {r4-r7}		\n"
		"mrs r3, msp		\n"
		"str r3, [%1]		\n" // %1 points to mspAtStart.
		"msr psp, %0		\n" // %0 points to the start of the exception frame.
		"movs r3, #2		\n" // Switch stacks.
		"msr control, r3	\n"
		"isb				\n"
		"pop {r0-r5}		\n" // In the frame there are r0-r3, r12, lr, pc
		"mov lr, r5			\n" // Here r0-r3 and lr is loaded, r12 is ignored.
		"cpsie i			\n"
		"pop {pc}			\n"
			: : "r" ((uint32_t*)task->sp + 4),/* Adjust sp to point to the exception frame*/
				"r" (&mspAtStart)
			: "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "lr", "sp", "pc", "memory"
	);
}

void ProfileCortexM0::finishLast(const char* ret)
{
	struct ReturnTaskStub {
		__attribute__((naked))
		static void restoreMasterState(const char* ret) {
			asm volatile (
				"ldr r3, [%0]		\n" // %1 points to the start of the exception frame.
				"msr msp, r3		\n"
				"movs r3, #0		\n"
				"msr control, r3	\n"
				"isb				\n"
				"pop {r4-r7}		\n"
				"mov r8, r4			\n"
				"mov r9, r5			\n"
				"mov r10, r6		\n"
				"mov r11, r7		\n"
				"pop {r4-r7, pc}	\n"
					: : "r" (&mspAtStart)
					: "r0", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "lr", "sp", "pc", "memory"
			);
		}
	};

	asm("cpsid i" ::: "memory"); // On the Cortex-M0 system interrupts can not be masked selectively.

	CortexCommon::Scb::Syst::disable();
	CortexCommon::Scb::Icsr::clearPendSV();

	const void **psp;
	asm volatile ("mrs %0, psp\n"  : "=r" (psp));
	psp[0] = reinterpret_cast<const void*>(ret);

	irqEntryStackedPc() = (void*)&ReturnTaskStub::restoreMasterState;
	currentTask = nullptr;

	asm("bx %0" :: "r" (0xFFFFFFFD): "memory");
}

void SVC_Handler() {
	CortexCommon::DirectSvc::dispatch(ProfileCortexM0::syncCallMapper);
}

void SysTick_Handler()
{
	ProfileCortexM0::tick++;
	ProfileCortexM0::tickHandler();
}

__attribute__((naked))
void PendSV_Handler() {

	asm volatile (
		".thumb					\n"
		".syntax unified		\n"

		"ldr r1, PendSV_async 	\n" // r0 = (void(*)()*) &asyncCallHandler
		"ldr r0, [r1]			\n" // r0 = (void(*)()) asyncCallHandler
		"cmp r0, #0				\n"	// if(asyncCallHandler) {
		"beq PendSV_noAsync		\n"	//

		"push {r1, lr}			\n"
		"blx r0					\n"	// asyncCallHandler();
		"pop {r1, r2}			\n"
		"mov lr, r2				\n"
		"movs r0, #0			\n"
		"str r0, [r1]			\n" // asyncCallHandler = nullptr;

		"PendSV_noAsync:		\n"	// }

		"ldr r0, PendSV_old		\n" // r0 = (Task**) &oldTask
		"ldr r1, [r0]			\n" // r1 = (Task*) oldTask

		"cmp r1, #0				\n"	// if(!oldTask)
		"beq PendSV_done		\n"	// 		goto done;

		"mrs r2, psp			\n" // r2 = psp
		"subs r2, r2, #32		\n" // r2 -= 8 * 4					// make room for additional registers
		"stmia r2!, {r4-r7}		\n" // *r2++ = {r4, r5, r6, r7} 	// save low regs

		"str r2, [r1]			\n"	// oldTask->sp = r2		// save mid-frame stack pointer
		"movs r3, #0			\n"
		"str r3, [r0]			\n" // *r0 = nullptr				// zero out oldTask

		"mov r4, r8				\n"
		"mov r5, r9				\n"
		"mov r6, r10			\n"
		"mov r7, r11			\n"
		"stmia r2!, {r4-r7}		\n" // *r2++ = {r8, r9, r10, r11} 	// store remaining high regs

		"ldr r0, PendSV_new		\n"	// r0 = (Task**) &oldTask
		"ldr r0, [r0]			\n" // r0 = oldTask
		"ldr r0, [r0]			\n" // r0 = oldTask->sp

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
			: "i" (&ProfileCortexM0::oldTask),
			  "i" (&ProfileCortexM0::currentTask),
			  "i" (&ProfileCortexM0::asyncCallHandler)
			: "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12"
	);
}
