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

#ifndef PROFILE_H_
#define PROFILE_H_

#include <stdint.h>
#include <malloc.h>
#include <ucontext.h>
#include <unistd.h>
#include <iostream>
#include <list>
#include <signal.h>
#include <string.h>
#include <sys/time.h>

#include <iostream>

namespace home {

class ProfileLinuxUm {
public:
	typedef uint32_t TickType;
	class Task {
		friend ProfileLinuxUm;
		ucontext_t exitContext;
		ucontext_t savedContext;
		uintptr_t exitStack[1024];
		uintptr_t returnValue;

		struct SyscallArgs {
			int nArgs;
			uintptr_t args[6];
		} syscallArgs;

		void (*syscall)(Task*);
	};

private:
	static void enableSignals();
	static void disableSignals();
	static void sigUsr1Handler(int sig);
	static void sigAlrmHandler(int);
	static void *defaultSyncCallMapper(void*);

	static void (* volatile asyncCallHandler)();
	static void* (* volatile syncCallMapper)(uintptr_t);
	static void (*tickHandler)();
	static volatile uint32_t tick;

	static Task* volatile currentTask;
	static Task* volatile oldTask;
	static bool suspended, suspend;
	static const char* returnValue;
	static ucontext_t final;
	static uintptr_t idleStack[1024];
	static ucontext_t idle;

	inline static Task* realCurrentTask() {
		return oldTask ? oldTask : currentTask;
	}

	static void dispatchSyscall(Task* self);
	static void idler();

public:
	static void init(uintptr_t tickUs);
	template<class ... T> static inline uintptr_t sync(T ... ops);
	static const char* startFirst(Task* task);
	static void finishLast(const char* errorMessage);
	static void initialize(Task* task, void (*entry)(void*), void (*exit)(), void* stack, uintptr_t stackSize, void* arg);

	static inline void async(void (*handler)()) {
		asyncCallHandler = handler;
		kill(getpid(), SIGUSR1);
	}

	static inline TickType getTick() {
		return tick;
	}

	typedef uint32_t ClockType;
	static inline ClockType getClockCycle() {
		return 0;
	}

	static inline void setTickHandler(void (*handler)()) {
		tickHandler = handler;
	}

	static inline void setSyscallMapper(void* (*mapper)(uintptr_t)) {
		syncCallMapper = mapper;
	}

	static inline Task* getCurrent() {
		if(suspended)
			return nullptr;

		return currentTask;
	}

	static inline void switchToAsync(Task* task) {
		if(!oldTask)
			oldTask = currentTask;

		currentTask = task;
	}

	static inline void switchToSync(Task* task) {
		switchToAsync(task);
	}

	static inline void injectReturnValue(Task* task, uintptr_t ret) {
		task->returnValue = ret;
	}

	static inline void suspendExecution() {
		suspend = true;
	}

	static inline void memoryFence() {
		asm("":::"memory");
	}
};

///////////////////////////////////////////////////////////////////////////////

template<class ... T>
inline uintptr_t ProfileLinuxUm::sync(T ... ops) {
	realCurrentTask()->syscallArgs.nArgs = sizeof...(T) - 1;
	uintptr_t tempArray[] = {((uintptr_t)ops)...};

	for(auto i = 0u; i < sizeof...(T); i++)
		realCurrentTask()->syscallArgs.args[i] = tempArray[i];

	realCurrentTask()->syscall = &dispatchSyscall;
	kill(getpid(), SIGUSR1);
	return realCurrentTask()->returnValue;
}

}

#endif /* PROFILE_H_ */
