/*
 * CallGate.h
 *
 *  Created on: 2017.05.27.
 *      Author: tooma
 */

#ifndef CALLGATE_H_
#define CALLGATE_H_

#include "Profile.h"

class ProfileCortexM0::CallGate {
	friend void PendSV_Handler();
	static void (* volatile asyncCallHandler)();

	static inline uintptr_t issueSvc(uintptr_t (*f)());
	static inline uintptr_t issueSvc(uintptr_t arg1, uintptr_t (*f)(uintptr_t));
	static inline uintptr_t issueSvc(uintptr_t arg1, uintptr_t arg2, uintptr_t (*f)(uintptr_t, uintptr_t));
	static inline uintptr_t issueSvc(uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t (*f)(uintptr_t, uintptr_t, uintptr_t));
	static inline uintptr_t issueSvc(uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4, uintptr_t (*f)(uintptr_t, uintptr_t, uintptr_t, uintptr_t));
	template<class ... Args> static inline uintptr_t callViaSvc(uintptr_t (f)(Args...), Args ... args);

public:
	template<class ... T> static inline uintptr_t sync(T ... ops);
	static inline void async(void (*asyncCallHandler)());
};

///////////////////////////////////////////////////////////////////////////////

inline void ProfileCortexM0::CallGate::async(void (*asyncCallHandler)()) {
	CallGate::asyncCallHandler = asyncCallHandler;
	Internals::Scb::Icsr::triggerPendSV();
}

template<class ... T>
inline uintptr_t ProfileCortexM0::CallGate::sync(T ... ops) {
	return callViaSvc(ops...);
}

template<class ... Args>
inline uintptr_t ProfileCortexM0::CallGate::callViaSvc(uintptr_t (f)(Args...), Args ... args) {
	return issueSvc((uintptr_t)args..., f);
}

inline uintptr_t ProfileCortexM0::CallGate::issueSvc(uintptr_t (*f)())
{
	register uintptr_t func asm("r12") = (uintptr_t)f;
	register uintptr_t ret asm("r0");

	asm volatile (
		"svc 0\n" : "=r"(ret) : "r"(func) :
	);

	return ret;
}

inline uintptr_t ProfileCortexM0::CallGate::issueSvc(uintptr_t arg1, uintptr_t (*f)(uintptr_t))
{
	register uintptr_t func asm("r12") = (uintptr_t)f;
	register uintptr_t arg_1 asm("r0") = (uintptr_t)arg1;
	register uintptr_t ret asm("r0");

	asm volatile (
		"svc 0\n" : "=r"(ret) : "r"(func), "r"(arg_1): "memory"
	);

	return ret;
}

inline uintptr_t ProfileCortexM0::CallGate::issueSvc(uintptr_t arg1, uintptr_t arg2, uintptr_t (*f)(uintptr_t, uintptr_t))
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

inline uintptr_t ProfileCortexM0::CallGate::issueSvc(uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t (*f)(uintptr_t, uintptr_t, uintptr_t))
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

inline uintptr_t ProfileCortexM0::CallGate::issueSvc(uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4, uintptr_t (*f)(uintptr_t, uintptr_t, uintptr_t, uintptr_t))
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


#endif /* CALLGATE_H_ */
