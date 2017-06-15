/*
 * CallGate.h
 *
 *  Created on: 2017.06.11.
 *      Author: tooma
 */

#ifndef CALLGATE_H_
#define CALLGATE_H_

#include "Profile.h"

#include <sys/types.h>
#include <signal.h>
#include <unistd.h>


class ProfileLinuxUm::CallGate {
	friend ProfileLinuxUm;

	static void (* volatile asyncCallHandler)();
	static void* (* volatile syncCallMapper)(void*);

	static void *defaultSyncCallMapper(void*);
public:
	static inline void setSyscallMapper(void* (*mapper)(void*)) {
		syncCallMapper = mapper;
	}

	template<class ... T>
	static inline uintptr_t sync(T ... ops) {
		Task::realCurrentTask()->syscallArgs.nArgs = sizeof...(T) - 1;
		uintptr_t tempArray[] = {((uintptr_t)ops)...};

		for(auto i = 0u; i < sizeof...(T); i++)
			Task::realCurrentTask()->syscallArgs.args[i] = tempArray[i];

		Task::realCurrentTask()->syscall = &Task::dispatchSyscall;
		kill(getpid(), SIGUSR1);
		return Task::realCurrentTask()->returnValue;
	}

	static inline void async(void (*handler)()) {
		asyncCallHandler = handler;
		kill(getpid(), SIGUSR1);
	}
};

#endif /* CALLGATE_H_ */
