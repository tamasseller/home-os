/*
 * Task.h
 *
 *  Created on: 2017.05.27.
 *      Author: tooma
 */

#ifndef PROFILE_TASK_H_
#define PROFILE_TASK_H_

#include "Profile.h"

#include <unistd.h>
#include <ucontext.h>

class ProfileLinuxUm::Task {
	friend ProfileLinuxUm;

	static Task* volatile currentTask;
	static Task* volatile oldTask;
	static bool suspended, suspend;
	static ucontext_t final;
	static uintptr_t idleStack[1024];
	static ucontext_t idle;

	static void idler()
	{
		sigset_t mask, oldMask;
		sigfillset(&mask);

		while(1) {
			sigprocmask(SIG_UNBLOCK, &mask, &oldMask);
			pause();
			sigprocmask(SIG_SETMASK, &oldMask, 0);
		}
	}

	ucontext_t exitContext;
	ucontext_t savedContext;

	uintptr_t exitStack[1024];

	uintptr_t returnValue;

	static Task* realCurrentTask() {
		return Task::oldTask ? Task::oldTask : Task::currentTask;
	}

	struct SyscallArgs {
		int nArgs;
		uintptr_t args[6];
	} syscallArgs;

	static void dispatchSyscall(Task* self) {
		uintptr_t *args = self->syscallArgs.args;
		void* functPtr = (void*)args[0];

		switch(self->syscallArgs.nArgs) {
		case 0:
			self->returnValue = ((uintptr_t (*)())functPtr)();
			break;
		case 1:
			self->returnValue = ((uintptr_t (*)(uintptr_t))functPtr)(args[1]);
			break;
		case 2:
			self->returnValue = ((uintptr_t (*)(uintptr_t, uintptr_t))functPtr)(args[1], args[2]);
			break;
		case 3:
			self->returnValue = ((uintptr_t (*)(uintptr_t, uintptr_t, uintptr_t))functPtr)(args[1], args[2], args[3]);
			break;
		case 4:
			self->returnValue = ((uintptr_t (*)(uintptr_t, uintptr_t, uintptr_t, uintptr_t))functPtr)(args[1], args[2], args[3], args[4]);
			break;
		}
	}

	void (*syscall)(Task*);

public:
	void startFirst();
	void finishLast();
	void initialize(void (*entry)(void*), void (*exit)(), void* stack, uint32_t stackSize, void* arg);

	inline void switchToAsync()
	{
		if(!oldTask)
			oldTask = currentTask;

		currentTask = this;
	}

	inline void switchToSync()
	{
		switchToAsync();
	}

	inline void injectReturnValue(uintptr_t ret)
	{
		returnValue = ret;
	}

	static inline Task* getCurrent()
	{
		if(suspended)
			return nullptr;

		return currentTask;
	}

	static inline void suspendExecution() {
		suspend = true;
	}
};


#endif /* PROFILE_TASK_H_ */
