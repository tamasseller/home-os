/*
 * Task.h
 *
 *  Created on: 2017.05.27.
 *      Author: tooma
 */

#ifndef PROFILE_TASK_H_
#define PROFILE_TASK_H_

#include "Profile.h"

class ProfileCortexM0::Task {
	friend void PendSV_Handler();
	static Task* volatile currentTask;
	static Task* volatile oldTask;

	template<class Type, void (Type::*entry)()> static void entryStub(Type* self);

	void* sp;

public:
	template<class Type, void (Type::*entry)(), void exit()>
	inline void initialize(void* stack, uint32_t stackSize, Type* arg);

	void startFirst();
	void finishLast();

	void switchTo();
	static Task* getCurrent();
};

///////////////////////////////////////////////////////////////////////////////

template<class Type, void (Type::*entry)()>
void ProfileCortexM0::Task::entryStub(Type* self) {
	(self->*entry)();
}

template<class Type, void (Type::*entry)(), void exit()>
inline void ProfileCortexM0::Task::initialize(void* stack, uint32_t stackSize, Type* arg) {
	uintptr_t* data = (uintptr_t*) stack + (stackSize / 4 - 1);

	void (*stub)(Type*) = &entryStub<Type, entry>;

	*data-- = 0x01000000;		// xpsr
	*data-- = (uintptr_t) stub;	// pc
	*data-- = (uintptr_t) exit;	// lr
	*data-- = 0xbaadf00d;		// r12
	*data-- = 0xbaadf00d;		// r3
	*data-- = 0xbaadf00d;		// r2
	*data-- = 0xbaadf00d;		// r1
	*data-- = (uintptr_t) arg;	// r0
	*data-- = 0xbaadf00d;		// r11
	*data-- = 0xbaadf00d;		// r10
	*data-- = 0xbaadf00d;		// r9
	*data-- = 0xbaadf00d;		// r8
	sp = data + 1;
	*data-- = 0xbaadf00d;		// r7
	*data-- = 0xbaadf00d;		// r6
	*data-- = 0xbaadf00d;		// r5
	*data-- = 0xbaadf00d;		// r4
}

inline void ProfileCortexM0::Task::switchTo() {
	oldTask = currentTask;
	currentTask = this;
	Internals::Scb::Icsr::triggerPendSV();
}

inline ProfileCortexM0::Task* ProfileCortexM0::Task::getCurrent() {
	return currentTask;
}


#endif /* PROFILE_TASK_H_ */
