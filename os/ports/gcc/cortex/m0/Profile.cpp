/*
 * Profile.cpp
 *
 *  Created on: 2017.05.24.
 *      Author: tooma
 */

#include "Profile.h"

volatile bool ProfileCortexM0::exclusiveMonitor = false;

volatile uint32_t ProfileCortexM0::Timer::tick = 0;
void (*ProfileCortexM0::Timer::tickHandler)();

volatile bool ProfileCortexM0::Internals::Scb::Icsr::isAsyncDeferred;
volatile bool isAsyncBlocked;
volatile uint32_t ProfileCortexM0::Internals::Scb::Icsr::asyncBlockCount;
void (* volatile ProfileCortexM0::CallGate::asyncCallHandler)();

ProfileCortexM0::Task* volatile ProfileCortexM0::Task::currentTask;
ProfileCortexM0::Task* volatile ProfileCortexM0::Task::oldTask;
void* ProfileCortexM0::Task::suspendedPc;


__attribute__((naked))
void ProfileCortexM0::Task::startFirst()
{
	currentTask = this;
	asm volatile (
		"cpsid i			\n" // On the Cortex-M0 system interrupts can not be masked selectively.
		"push {r4-r7, lr}	\n" // ABI: r0-r3 and r12 can be destroyed during a procedure call.
		"mov r4, r8			\n" // r4-r11 and lr are stored here in a platform-convenient order.
		"mov r5, r9			\n" // pc will be restored by loading lr (i.e. returning)
		"mov r6, r10		\n" // sp is saved implicitly by keeping it in MSP, and using
		"mov r7, r11		\n" // PSP for the threads (interrupts will restore MSP on exit).
		"push {r4-r7}		\n"
		"msr psp, %0		\n" // %0 points to the start of the exception frame.
		"movs r0, #2		\n" // Switch tasks.
		"msr control, r0	\n"
		"isb				\n"
		"pop {r0-r5}		\n" // In the frame there are r0-r3, r12, lr, pc
		"mov lr, r5			\n" // Here r0-r3 and lr is loaded, r12 is ignored.
		"cpsie i			\n"
		"pop {pc}			\n"
			: : "r" ((uint32_t*)sp + 4) /* Adjust sp to point to the exception frame*/:
	);
}

void ProfileCortexM0::Task::finishLast()
{
	struct ReturnTaskStub {
		__attribute__((naked))
		static void restoreMasterState() {
			asm volatile (
				"cpsid i			\n" // On the Cortex-M0 system interrupts can not be masked selectively.
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

	irqEntryStackedPc() = (void*)&ReturnTaskStub::restoreMasterState;
	currentTask = nullptr;
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

	asm volatile (
		".thumb					\n"
		".syntax unified		\n"

		"ldr r1, PendSV_async 	\n" // r0 = (void(*)()*) &CallGate::asyncCallHandler
		"ldr r0, [r1]			\n" // r0 = (void(*)()) CallGate::asyncCallHandler
		"cmp r0, #0				\n"	// if(CallGate::asyncCallHandler) {
		"beq PendSV_noAsync		\n"	//

		"push {r1, lr}			\n"
		"blx r0					\n"	// CallGate::asyncCallHandler();
		"pop {r1, r2}			\n"
		"mov lr, r2				\n"
		"movs r0, #0			\n"
		"str r0, [r1]			\n" // CallGate::asyncCallHandler = nullptr;

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
