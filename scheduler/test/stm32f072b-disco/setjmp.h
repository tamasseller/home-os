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
