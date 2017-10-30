/*
 * Syscall.h
 *
 *  Created on: 2017.10.30.
 *      Author: tooma
 */

#ifndef SYSCALL_H_
#define SYSCALL_H_

template<class... Args>
template<class...> class Scheduler<Args...>::SyscallMap
{

};

template<class... Args>
inline void* Scheduler<Args...>::syscallMapper(uintptr_t p)
{
	return reinterpret_cast<void*>(p);
}

template<class... Args>
template<class M, class... T>
inline uintptr_t Scheduler<Args...>::syscall(M method, T... ops)
{
	return Profile::sync(method, ops...);
}

template<class... Args>
template<class M, class... T>
inline uintptr_t Scheduler<Args...>::conditionalSyscall(M method, T... ops)
{
	if(state.isRunning)
		return syscall(method, ops...);
	else
		return method(ops...);
}

#endif /* SYSCALL_H_ */
