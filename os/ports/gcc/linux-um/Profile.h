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
#include <iostream>
#include <list>
#include <signal.h>
#include <string.h>
#include <sys/time.h>

#include <iostream>

class ProfileLinuxUm {
	static void sigUsr1Handler(int sig);
public:
	class Task;
	class Timer;
	class CallGate;
	template<class Data> class Atomic;

	static void init(uintptr_t tickUs);

	static inline void fatalError(const char* msg);
};

#include "Task.h"
#include "Timer.h"
#include "Atomic.h"
#include "CallGate.h"

///////////////////////////////////////////////////////////////////////////////

inline void ProfileLinuxUm::fatalError(const char* msg)
{
	std::cerr << "Fatal error: " << msg << "!" << std::endl;
}

#endif /* PROFILE_H_ */
