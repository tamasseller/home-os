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

#ifndef SVC_H_
#define SVC_H_

namespace home {

namespace CortexCommon {

struct DirectSvc {
	inline static uintptr_t issueSvc(uintptr_t f)
	{
		register uintptr_t func asm("r12") = (uintptr_t)f;
		register uintptr_t ret asm("r0");

		asm volatile (
			"svc 0\n" : "=r"(ret) : "r"(func) :
		);

		return ret;
	}

	inline static uintptr_t issueSvc(uintptr_t f, uintptr_t arg1)
	{
		register uintptr_t func asm("r12") = (uintptr_t)f;
		register uintptr_t arg_1 asm("r0") = (uintptr_t)arg1;
		register uintptr_t ret asm("r0");

		asm volatile (
			"svc 0\n" : "=r"(ret) : "r"(func), "r"(arg_1): "memory"
		);

		return ret;
	}

	inline static uintptr_t issueSvc(uintptr_t f, uintptr_t arg1, uintptr_t arg2)
	{
		register uintptr_t func asm("r12") = (uintptr_t)f;
		register uintptr_t arg_1 asm("r0") = (uintptr_t)arg1;
		register uintptr_t arg_2 asm("r1") = (uintptr_t)arg2;
		register uintptr_t ret asm("r0");

		asm volatile (
			"svc 0\n" : "=r"(ret) : "r"(func), "r"(arg_1), "r"(arg_2): "memory"
		);

		return ret;
	}

	inline static uintptr_t issueSvc(uintptr_t f, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3)
	{
		register uintptr_t func asm("r12") = (uintptr_t)f;
		register uintptr_t arg_1 asm("r0") = (uintptr_t)arg1;
		register uintptr_t arg_2 asm("r1") = (uintptr_t)arg2;
		register uintptr_t arg_3 asm("r2") = (uintptr_t)arg3;
		register uintptr_t ret asm("r0");

		asm volatile (
			"svc 0\n" : "=r"(ret) : "r"(func), "r"(arg_1), "r"(arg_2), "r"(arg_3): "memory"
		);

		return ret;
	}

	inline static uintptr_t issueSvc(uintptr_t f, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4)
	{
		register uintptr_t func asm("r12") = (uintptr_t)f;
		register uintptr_t arg_1 asm("r0") = (uintptr_t)arg1;
		register uintptr_t arg_2 asm("r1") = (uintptr_t)arg2;
		register uintptr_t arg_3 asm("r2") = (uintptr_t)arg3;
		register uintptr_t arg_4 asm("r3") = (uintptr_t)arg4;
		register uintptr_t ret asm("r0");

		asm volatile (
			"svc 0\n" : "=r"(ret) : "r"(func), "r"(arg_1), "r"(arg_2), "r"(arg_3), "r"(arg_4): "memory"
		);

		return ret;
	}

	inline static void dispatch(register void* (*mapper)(uintptr_t))
	{
		uintptr_t* psp;
		asm volatile ("mrs %[psp], psp" : [psp] "=r" (psp) : : );

		void* handler = mapper(psp[4]); // Lookup r12

		asm volatile (
			".thumb						\n"
			".syntax unified			\n"
			"ldmia %[psp]!, {r0-r3}	  	\n" // Load stored regs (r0, r1, r2, r3)
			"blx %[handler]			  	\n"
			"subs %[psp], #16 			\n"
			"str r0, [%[psp]]		 	\n"
				:
				: [psp] "r" (psp), [handler] "r" (handler)
				: "r0", "r1", "r2", "r3", "lr", "cc"
		);
	}
};

}

}

#endif /* SVC_H_ */
