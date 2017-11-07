/*
 * ErrorStrings.h
 *
 *  Created on: 2017.11.03.
 *      Author: tooma
 */

#ifndef ERRORSTRINGS_H_
#define ERRORSTRINGS_H_

template<class... Args>
struct Scheduler<Args...>::ErrorStrings {
	static constexpr const char* unknownError = "WTF internal error";
	static constexpr const char* unreachableReached = "Internal error, this method should never have been called.";
	static constexpr const char* interruptOverload = "Tick overload, preemption event could not have been dispatched for a full tick cycle!";

	static constexpr const char* objectAlreadyRegistered = "Object registered multiple times";
	static constexpr const char* invalidSyscall = "Unknown system call dispatched";
	static constexpr const char* invalidSyscallArgument = "Invalid object pointer as syscall argument";

	static constexpr const char* policyBlockerUsage = "Only priority change can be handled through the Blocker interface of the Policy wrapper";
	static constexpr const char* policyNonTask = "Only tasks can be handled by the policy container";

	static constexpr const char* mutexBlockerUsage = "Only priority change can be handled through the Blocker interface of a Mutex";
	static constexpr const char* mutexNonOwnerUnlock = "Mutex unlock from non-owner task";
	static constexpr const char* mutexAsyncUnlock = "Asynchronous mutex unlock";
	static constexpr const char* mutexDeadlock = "Blocking dependency loop detected while acquiring lock";

	static constexpr const char* taskDelayTooBig = "Delay time too big!";

	static constexpr const char* ioRequestReuse = "Attempt to reuse an already occupied io request object";
};

#endif /* ERRORSTRINGS_H_ */
