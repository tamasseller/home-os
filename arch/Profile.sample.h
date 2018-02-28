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

#ifndef PROFILE_SAMPLE_H_
#define PROFILE_SAMPLE_H_

namespace home {

/**
 * Hardware profile documentation sample.
 *
 * There is a common interface for the hardware dependent code,
 * whose requirements must be satisfied by compatible ports.
 *
 * The availability of the methods of the platform interface
 * depend on the execution context, there are three of them:
 *
 *  - Task context, regular application task code;
 *  - System calls (synchronous and asynchronous).
 *  - Interrupt context, interrupts of the application code;
 *
 * Involuntary switching between these contexts can occur only
 * in the following ways:
 *
 *  - Tasks can be preempted by system calls, regular interrupt
 *    and also by other Tasks. Task mode can be entered only
 *    through system call context.
 *  - System calls can only be preempted by regular interrupts,
 *    but neither tasks nor other system calls. System call
 *    context can be entered in two ways, through a direct
 *    syscall from task mode or an asynchronous syscall
 *    dispatch request, that can be triggered from any contexts.
 *  - Regular interrupts can do whatever the platform defines,
 *    the scheduler has nothing to do about them.
 */
class ProfileSample {
	/**
	 * Platform specific task state.
	 *
	 * The scheduler core does not observe or modify these data directly.
	 *
	 * Task objects are guaranteed to be initialized before the first use,
	 * via the implementation defined _initialize_ method and then exactly
	 * one of them are selected for execution using one of the _startFirst_,
	 * _switchToSync_, _switchToAsync_, _suspendExecution_ and _finishLast_
	 * methods, described below.
	 */
	class Task;

	/**
	 * Get the currently running task.
	 *
	 * The implementation is responsible for keeping track of the currently
	 * running task, to allow for platform specific optimizations. This method
	 * is required to return a pointer to the currently running task object.
	 * If the execution is suspended it must return nullptr.
	 *
	 * The
	 */
	static Task* getCurrent();

	/**
	 * Initialize the platform specific task state according to the following:
	 *
	 *  - once the task is started it must enter the function pointed to by
	 *    _entry_ with its argument being _arg_,
	 *  - if the _entry_ function returns it must do so by entering the function
	 *    pointed to by _exit_.
	 *  - the stack space used for the thread is pointed to by _stack_ and has
	 *    the size of _stackSize_ bytes.
	 */
	static void initialize(Task*, void (*entry)(void*), void (*exit)(), void* stack, uint32_t stackSize, void* arg);

	/**
	 * Swap the current task.
	 *
	 * This method is required to exchange the active task context with the one
	 * pointed to by _task_.
	 *
	 * This method is guaranteed to be called from **synchronous** system call context only.
	 *
	 * NOTE: This method is called only as the last action in syscall handlers, so
	 *       that it is allowed to omit returning in the regular way.
	 */
	static void switchToSync(Task* task);

	/**
	 * Swap the current task, from asynchronous syscall.
	 *
	 * The requirements for this method are the same as for the _swithToSync_ method, but
	 * this method is guaranteed to be called only from **asynchronous** system call context.
	 *
	 * NOTE: This method is called only as the last action in syscall handlers, so
	 *       that it is allowed to omit returning in the regular way.
	 */
	static void switchToAsync(Task* task);

	/**
	 * Start the first task.
	 *
	 * Switch to the first task, whose platform specific state is pointed to by _task_.
	 */
	static const char* startFirst(Task* task);

	/**
	 * Quit the scheduler.
	 *
	 * Calling this method is the last action of the scheduler core upon exiting.
	 * It is required to restore the state that startFirst observed.
	 */
	static void finishLast(const char*);

	/**
	 * Suspend the task execution temporarily.
	 */
	static void suspendExecution();

	struct uint32_t TickType;
	static TickType getTick();

	struct uint32_t ClockType;
	static ClockType getClockCycle();

	static void setSyscallMapper(void* (*mapper)(void*));
	template<class ... T> static inline uintptr_t sync(T ... ops);
	static void async(void (*handler)());

	static void init(/* platform dependent */);

	static inline void memoryFence();
};

}

#endif /* PROFILE_SAMPLE_H_ */
