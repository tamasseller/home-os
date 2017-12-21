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

#ifndef SYSCALL_H_
#define SYSCALL_H_

namespace home {

namespace detail {

template<class C, C c>
struct Syscall{
        static constexpr auto target = c;
        template<class O, class X = void> struct Comparator;
};

template<class... Cs>
struct SyscallDb {
    template<class, class... List> struct IdOfWorker;

    static constexpr uintptr_t nCalls = sizeof...(Cs);
    static constexpr void* targets[nCalls] = {(void*)Cs::target...};

    template<class C>
    using IdOf = IdOfWorker<C, Cs...>;
};

template<class... Cs>
constexpr void* SyscallDb<Cs...>::targets[];

template<class... Cs>
template<class X, class Current, class... Rest>
struct SyscallDb<Cs...>::IdOfWorker<X, Current, Rest...> {
    static constexpr uintptr_t value = (IdOfWorker<X, Rest...>::value == (uintptr_t)-1) ? -1 : (IdOfWorker<X, Rest...>::value + 1);
};

template<class... Cs>
template<class X, class... Rest>
struct SyscallDb<Cs...>::IdOfWorker<X, X, Rest...> {
    static constexpr uintptr_t value = 0;
};

template<class... Cs>
template<class X>
struct SyscallDb<Cs...>::IdOfWorker<X> {
    static constexpr uintptr_t value = -1;
};

}

/*
 * This macro is provided to shorten the rather lengthy description of the syscall handle,
 * the verbose syntax is necessary due to the properties of the language (could only be
 * avoided with auto typed template parameter).
 */
#define SYSCALL(x) detail::Syscall<decltype(&Scheduler<Args...>:: x ), &Scheduler<Args...>:: x >

template<class... Args>
struct Scheduler<Args...>::SyscallMap: detail::SyscallDb<
    SYSCALL(doStartTask),
    SYSCALL(doExit),
    SYSCALL(doAbort),
    SYSCALL(doYield),
    SYSCALL(doSleep),

    SYSCALL(doRegisterObject<Mutex>),
    SYSCALL(doUnregisterObject<Mutex>),
    SYSCALL(doBlock<Mutex>),
    SYSCALL(doRelease<Mutex>),

    SYSCALL(doRegisterObject<BinarySemaphore>),
    SYSCALL(doUnregisterObject<BinarySemaphore>),
    SYSCALL(doBlock<BinarySemaphore>),
    SYSCALL(doTimedBlock<BinarySemaphore>),
    SYSCALL(doRelease<BinarySemaphore>),

    SYSCALL(doRegisterObject<CountingSemaphore>),
    SYSCALL(doUnregisterObject<CountingSemaphore>),
    SYSCALL(doBlock<CountingSemaphore>),
    SYSCALL(doTimedBlock<CountingSemaphore>),
    SYSCALL(doRelease<CountingSemaphore>),

    SYSCALL(doRegisterObject<WaitableSet>),
    SYSCALL(doUnregisterObject<WaitableSet>),
    SYSCALL(doBlock<WaitableSet>),
    SYSCALL(doTimedBlock<WaitableSet>),

    SYSCALL(doRegisterObject<IoChannel>),
    SYSCALL(doUnregisterObject<IoChannel>),

    SYSCALL(doRegisterObject<IoRequestCommon>),
    SYSCALL(doUnregisterObject<IoRequestCommon>),
    SYSCALL(doBlock<IoRequestCommon>),
    SYSCALL(doTimedBlock<IoRequestCommon>)
>{};

template<class... Args>
inline void* Scheduler<Args...>::syscallMapper(uintptr_t p)
{
    assert(p < SyscallMap::nCalls, ErrorStrings::invalidSyscall);
	return SyscallMap::targets[p];
}

template<class... Args>
template<class Syscall, class... T>
inline uintptr_t Scheduler<Args...>::syscall(T... ops)
{
    static constexpr uintptr_t id = SyscallMap::template IdOf<Syscall>::value;
    static_assert(id != (uintptr_t)-1, "Unknown system call issued");
	return Profile::sync(id, ops...);
}

template<class... Args>
template<class Syscall, class... T>
inline uintptr_t Scheduler<Args...>::conditionalSyscall(T... ops)
{
	if(state.isRunning)
		return syscall<Syscall>(ops...);
	else
		return Syscall::target(ops...);
}

}

#endif /* SYSCALL_H_ */
