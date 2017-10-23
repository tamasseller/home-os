/*
 * setjmp.h
 *
 *  Created on: 2017.05.29.
 *      Author: tooma
 */

#ifndef SETJMP_H_
#define SETJMP_H_

#include <stdint.h>

extern "C" {

typedef struct {
	uintptr_t regs[10];
} jmp_buf;

int __setjmp (jmp_buf *env);
void __longjmp (jmp_buf *env, int val);

#define setjmp(x) 		__setjmp(&x)
#define longjmp(x, y)	__longjmp(&x, y)


}


#endif /* SETJMP_H_ */
