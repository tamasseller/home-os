/*
 * CallGate.h
 *
 *  Created on: 2017.05.27.
 *      Author: tooma
 */

#ifndef CALLGATE_H_
#define CALLGATE_H_

#include "Profile.h"

#include "../common/Svc.h"

class ProfileCortexM0::CallGate {
	friend void PendSV_Handler();
	friend void SVC_Handler();
	static void (* volatile asyncCallHandler)();
	static void* (* volatile syncCallMapper)(void*);

	static void *defaultSyncCallMapper(void*);

	template<class ... Args>
	static inline uintptr_t callViaSvc(uintptr_t (f)(Args...), Args ... args) {
		return CortexCommon::DirectSvc::issueSvc((uintptr_t)args..., f);
	}

public:

	template<class ... T>
	static inline uintptr_t sync(T ... ops) {
		return callViaSvc(ops...);
	}

	static inline void async(void (*asyncCallHandler)()) {
		CallGate::asyncCallHandler = asyncCallHandler;
		CortexCommon::Scb::Icsr::triggerPendSV();
	}
};


#endif /* CALLGATE_H_ */
