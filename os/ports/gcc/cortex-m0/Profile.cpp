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
		"msr psp, %0		\n"
		"movs r0, #2		\n"
		"msr CONTROL, r0	\n"
		"pop {r0-r5}		\n"
		"mov lr, r5			\n"
		"cpsie i			\n"
		"pop {pc}			\n"
			: /* No outputs */
			: "r" ((uint32_t*)sp + 4)
			: /* No clobbers */
	);
}

uintptr_t ProfileCortexM0::CallGate::issueSvc(uintptr_t (*f)())
{
	asm volatile (
		"mov r12, r0\n"
		"svc 0\n"
		"bx lr\n" : : : "r12"
	);
}

uintptr_t ProfileCortexM0::CallGate::issueSvc(uintptr_t arg1, uintptr_t (*f)(uintptr_t))
{
	asm volatile (
		"mov r12, r1\n"
		"svc 0\n"
		"bx lr\n" : : : "r12"
	);
}

uintptr_t ProfileCortexM0::CallGate::issueSvc(uintptr_t arg1, uintptr_t arg2, uintptr_t (*f)(uintptr_t, uintptr_t))
{
	asm volatile (
		"mov r12, r2\n"
		"svc 0\n"
		"bx lr\n" : : : "r12"
	);
}

uintptr_t ProfileCortexM0::CallGate::issueSvc(uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t (*f)(uintptr_t, uintptr_t, uintptr_t))
{
	asm volatile (
		"mov r12, r3\n"
		"svc 0\n"
		"bx lr\n" : : : "r12"
	);
}

uintptr_t ProfileCortexM0::CallGate::issueSvc(uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4, uintptr_t (*f)(uintptr_t, uintptr_t, uintptr_t, uintptr_t))
{
	asm volatile (
		"ldr r4, [sp, #0]\n"
		"mov r4, r12\n"
		"svc 0\n"
		"bx lr\n" : : : "r4", "12"
	);
}

