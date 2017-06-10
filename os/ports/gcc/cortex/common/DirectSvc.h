/*
 * DirectSvc.h
 *
 *  Created on: 2017.06.10.
 *      Author: tooma
 */

#ifndef DIRECTSVC_H_
#define DIRECTSVC_H_

namespace CortexCommon {

struct DirectSvc {
	inline static uintptr_t issueSvc(uintptr_t (*f)())
	{
		register uintptr_t func asm("r12") = (uintptr_t)f;
		register uintptr_t ret asm("r0");

		asm volatile (
			"svc 0\n" : "=r"(ret) : "r"(func) :
		);

		return ret;
	}

	inline static uintptr_t issueSvc(uintptr_t arg1, uintptr_t (*f)(uintptr_t))
	{
		register uintptr_t func asm("r12") = (uintptr_t)f;
		register uintptr_t arg_1 asm("r0") = (uintptr_t)arg1;
		register uintptr_t ret asm("r0");

		asm volatile (
			"svc 0\n" : "=r"(ret) : "r"(func), "r"(arg_1): "memory"
		);

		return ret;
	}

	inline static uintptr_t issueSvc(uintptr_t arg1, uintptr_t arg2, uintptr_t (*f)(uintptr_t, uintptr_t))
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

	inline static uintptr_t issueSvc(uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t (*f)(uintptr_t, uintptr_t, uintptr_t))
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

	inline static uintptr_t issueSvc(uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4, uintptr_t (*f)(uintptr_t, uintptr_t, uintptr_t, uintptr_t))
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

	inline static void dispatch() {
		asm volatile (
			"mrs r4, psp		\n"
			"ldmia r4, {r0-r4}	\n" // Load stored regs (r0, r1, r2, r3, r12)
			"blx r4				\n"	// Call to the address stored in the r12 of the caller
			"mrs r1, psp		\n" // Save r0 into the frame of the caller
			"str r0, [r1, #0]	\n" : : : "r0", "r1", "r2", "r3", "r4", "lr"
		);
	}
};

}




#endif /* DIRECTSVC_H_ */
