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
	static void (*asyncCallHandler)();

	static uintptr_t issueSvc(uintptr_t (*f)());
	static uintptr_t issueSvc(uintptr_t arg1, uintptr_t (*f)(uintptr_t));
	static uintptr_t issueSvc(uintptr_t arg1, uintptr_t arg2, uintptr_t (*f)(uintptr_t, uintptr_t));
	static uintptr_t issueSvc(uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t (*f)(uintptr_t, uintptr_t, uintptr_t));
	static uintptr_t issueSvc(uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4, uintptr_t (*f)(uintptr_t, uintptr_t, uintptr_t, uintptr_t));
	template<class ... Args> static inline uintptr_t callViaSvc(uintptr_t (f)(Args...), Args ... args);

public:
	template<class ... T> static inline uintptr_t sync(T ... ops);
	static inline void async(void (*asyncCallHandler)());
};

///////////////////////////////////////////////////////////////////////////////

template<class ... Args>
inline uintptr_t ProfileCortexM0::CallGate::callViaSvc(uintptr_t (f)(Args...), Args ... args) {
	return issueSvc((uintptr_t)args..., f);
}
template<class ... T>
inline uintptr_t ProfileCortexM0::CallGate::sync(T ... ops) {
	return callViaSvc(ops...);
}

inline void ProfileCortexM0::CallGate::async(void (*asyncCallHandler)()) {
	CallGate::asyncCallHandler = asyncCallHandler;
	Internals::Scb::Icsr::triggerPendSV();
}

#endif /* CALLGATE_H_ */
