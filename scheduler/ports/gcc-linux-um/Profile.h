/*
 * Profile.h
 *
 *  Created on: 2017.05.24.
 *      Author: tooma
 */

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

class ProfileLinuxUm {
public:
	template<class Data> class Atomic;
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

	static inline void fatalError(const char* msg) {
		std::cerr << "Fatal error: " << msg << "!" << std::endl;
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

template<class Data>
class ProfileLinuxUm::Atomic {
	volatile Data data;
public:
	inline Atomic(): data(0) {}

	inline Atomic(const Data& value) {
		data = value;
	}

	inline operator Data() {
		return data;
	}

	template<class Op, class... Args>
	inline Data operator()(Op&& op, Args... args)
	{
		Data old, result;

		do {
			old = this->data;

			if(!op(old, result, args...))
				break;

		} while(!__sync_bool_compare_and_swap(&this->data, old, result));

		return old;
	}
};


#endif /* PROFILE_H_ */
