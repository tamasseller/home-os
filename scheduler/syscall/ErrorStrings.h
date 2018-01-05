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

#ifndef ERRORSTRINGS_H_
#define ERRORSTRINGS_H_

namespace home {

template<class... Args>
struct Scheduler<Args...>::ErrorStrings {
	static constexpr const char* unknownError = "WTF internal error";
	static constexpr const char* unreachableReached = "Internal error, this method should never have been called";
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

	static constexpr const char* ioChannelDelete = "I/O channels can not be deleted with an active scheduler";

	static constexpr const char* ioJobDelete = "I/O jobs can not be deleted while running";

	static constexpr const char* ioRequestReuse = "Attempt to reuse an already occupied I/O request object";
	static constexpr const char* ioRequestState = "Internal error, invalid I/O operation state";
	static constexpr const char* ioRequestArgument = "Internal error, invalid I/O operation argument";
};

}

#endif /* ERRORSTRINGS_H_ */
