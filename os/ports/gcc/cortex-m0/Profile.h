/*
 * CortexM0.h
 *
 *  Created on: 2017.05.24.
 *      Author: tooma
 */

#ifndef CORTEXM0_H_
#define CORTEXM0_H_

#include <stdint.h>

class ProfileCortexM0 {
	template<class, template<class > class> friend class Scheduler;

	class Task;
	class Timer;
	class CallGate;
	template<class> class Atomic;

public:
	class Internals;

	static inline void init(uint32_t ticks);
};

#include "Internals.h"
#include "Task.h"
#include "Timer.h"
#include "Atomic.h"
#include "CallGate.h"

///////////////////////////////////////////////////////////////////////////////

inline void ProfileCortexM0::init(uint32_t ticks) {
	Internals::Scb::Syst::init(ticks);
	Internals::Scb::Shpr::init(0, 0 /* 0x40 */, 0);
}

#endif /* CORTEXM0_H_ */
