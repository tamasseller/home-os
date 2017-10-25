/*
 * Profile.sample.h
 *
 *  Created on: 2017.10.23.
 *      Author: tooma
 */

#ifndef PROFILE_SAMPLE_H_
#define PROFILE_SAMPLE_H_

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
	 * Atomically modifiable data container.
	 *
	 * The platform implementation is required to provide
	 * a solution, that is usable from any contexts.
	 *
	 * The _Data_ parameter is at most native machine
	 * word sized.
	 */
	template<class Data> class Atomic {
		/**
		 * Zeroing constructor.
		 *
		 * The default constructor is required to
		 * store the adequate null value.
		 */
		Atomic();

		/**
		 * Non-atomic copy.
		 *
		 * The default copy constructor is not required
		 * to implement atomic access behavior.
		 */
		Atomic(const Data& value);

		/**
		 * Non-atomic getter.
		 */
		operator Data();

		/**
		 * Arbitrary atomic modification.
		 *
		 * The platform implementation is required to execute
		 * the op functor atomically with the following constraints:
		 *
		 *  - it has to provide the old value as the first argument;
		 *  - it has to provide a valid, non-const, lvalue-reference to
		 *    a _Data_ instance that is filled with the new value by the
		 *    functor, as the second argument;
		 *  - it has to forward the additional arguments to the functor;
		 *  - if the boolean return value of the functor is true, it must
		 *    try to apply the new value, otherwise it must abort the
		 *    operation and leave the stored value unchanged;
		 *  - the implementation may restart the whole process (to allow
		 *    for the case of contention);
		 *  - after the operation is finished, it must return the
		 *    resulting value.
		 */
		template<class Op, class... Args> Data operator()(Op&& op, Args... args);
	};

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
	 *
	 * NOTE: This method is called only as the last action in syscall handlers, so
	 *       that it is allowed to omit returning in the regular way.
	 */
	static void startFirst(Task* task);

	/**
	 * Quit the scheduler.
	 *
	 * Calling this method is the last action of the scheduler core upon exiting.
	 * It is required to restore the state that startFirst observed.
	 */
	static void finishLast();

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
	static void fatalError(const char* msg);
};



#endif /* PROFILE_SAMPLE_H_ */
