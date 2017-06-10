/*
 * Timer.h
 *
 *  Created on: 2017.05.27.
 *      Author: tooma
 */

#ifndef TIMER_H_
#define TIMER_H_

#include "Profile.h"

class ProfileCortexM0::Timer {
	friend ProfileCortexM0;
	friend void SysTick_Handler();

	static void (*tickHandler)();

	static volatile uint32_t tick;
public:
	typedef uint32_t TickType;
	static inline TickType getTick();

	typedef uint32_t ClockType;
	static inline ClockType getClockCycle();

	static inline void setTickHandler(void (*tickHandler)());
};

///////////////////////////////////////////////////////////////////////////////

inline ProfileCortexM0::Timer::TickType ProfileCortexM0::Timer::getTick() {
	return tick;
}

inline ProfileCortexM0::Timer::ClockType ProfileCortexM0::Timer::getClockCycle() {
	return 0;
}

inline void ProfileCortexM0::Timer::setTickHandler(void (*tickHandler)()) {
	ProfileCortexM0::Timer::tickHandler = tickHandler;
}

#endif /* TIMER_H_ */
