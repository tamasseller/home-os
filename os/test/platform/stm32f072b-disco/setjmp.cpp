/*
 * setjmp.c
 *
 *  Created on: 2017.05.29.
 *      Author: tooma
 */

// Implementation borrowed from newlib.

#include "setjmp.h"

extern "C" {

__attribute__((naked))
int __setjmp(jmp_buf *env) {
	asm(

		/* Save registers in jump buffer.  */

		"stmia	r0!, {r4-r7}	\n"
		"mov	r1, r8			\n"
		"mov	r2, r9			\n"
		"mov	r3, r10			\n"
		"mov	r4, fp			\n"
		"mov	r5, sp			\n"
		"mov	r6, lr			\n"
		"stmia	r0!, {r1-r6}	\n"
		"sub	r0, r0, #40		\n"

		/* Restore callee-saved low regs.  */

		"ldmia	r0!, {r4-r7}	\n"

		/* Return zero.  */

		"mov	r0, #0			\n"
		"bx lr					\n"
	);

}

__attribute__((naked))
void __longjmp(jmp_buf *env, int val) {
	asm(
		"	add	r0, r0, #16		\n"
		"	ldmia r0!, {r2-r6}	\n"
		"	mov	r8, r2			\n"
		"	mov	r9, r3			\n"
		"	mov	r10, r4			\n"
		"	mov	fp, r5			\n"
		"	mov	sp, r6			\n"
		"	ldmia	r0!, {r3}	\n"  /* lr */

		/* Restore low regs.  */

		"	sub	r0, r0, #40		\n"
		"	ldmia r0!, {r4-r7}	\n"

		/* Return the result argument, or 1 if it is zero.  */

		"	mov	r0, r1			\n"
		"	bne	1f				\n"
		"	mov	r0, #1			\n"
		"1:						\n"
		"	bx	r3				\n"
	);
}

}

