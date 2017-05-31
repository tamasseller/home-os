/*
 * CortexM0.h
 *
 *  Created on: 2017.05.24.
 *      Author: tooma
 */

#ifndef CORTEXM0_H_
#define CORTEXM0_H_

#include <stdint.h>

struct ProfileCortexM0 {
	class Task;
	class Timer;
	class CallGate;
	template<class> class Atomic;

	class Internals;

	static inline void init(uint32_t ticks, uint8_t systickPrio=0xc0);
};

#include "Internals.h"
#include "Task.h"
#include "Timer.h"
#include "Atomic.h"
#include "CallGate.h"

///////////////////////////////////////////////////////////////////////////////

inline void ProfileCortexM0::init(uint32_t ticks, uint8_t systickPrio) {
	Internals::Scb::Syst::init(ticks);
	Internals::Scb::Shpr::init(0xc0, systickPrio, 0xc0);
}

#endif /* CORTEXM0_H_ */
