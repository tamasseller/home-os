/*
 * Sleeper.h
 *
 *  Created on: 2017.05.28.
 *      Author: tooma
 */

#ifndef SLEEPER_H_
#define SLEEPER_H_

#include "Scheduler.h"

template<class Profile, template<class> class PolicyParam>
class Scheduler<Profile, PolicyParam>::Sleeper {
	static constexpr Sleeper *invalid = (Sleeper *)0xffffffff;

	friend pet::DoubleList<Sleeper>;
	Sleeper *prev = invalid, *next;
public:
	uintptr_t deadline;

	inline bool operator < (uintptr_t time) const {
		return (intptr_t)(deadline - time) < 0;
	}

	inline bool operator < (const Sleeper& other) const {
		return *this < other.deadline;
	}

	inline bool isSleeping() {
		return prev == invalid;
	}

	inline void invalidate() {
		prev = invalid;
	}
};

#endif /* SLEEPER_H_ */
