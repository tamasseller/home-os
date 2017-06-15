/*
 * Timer.h
 *
 *  Created on: 2017.06.11.
 *      Author: tooma
 */

#ifndef TIMER_H_
#define TIMER_H_

#include "Profile.h"

class ProfileLinuxUm::Timer {
	friend ProfileLinuxUm;

	static void (*tickHandler)();
	static volatile uint32_t tick;

	static void sigAlrmHandler(int);
public:
	typedef uint32_t TickType;
	static inline TickType getTick() {
		return tick;
	}

	typedef uint32_t ClockType;
	static inline ClockType getClockCycle() {
		return 0;
	}

	static inline void setTickHandler(void (*tickHandler)()) {
		ProfileLinuxUm::Timer::tickHandler = tickHandler;
	}
};


#endif /* TIMER_H_ */
