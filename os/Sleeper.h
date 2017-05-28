/*
 * Sleeper.h
 *
 *  Created on: 2017.05.28.
 *      Author: tooma
 */

#ifndef SLEEPER_H_
#define SLEEPER_H_

#include "Scheduler.h"

template<class Profile, template<class> class Policy>
class Scheduler<Profile, Policy>::Sleeper {
	friend pet::DoubleList<Sleeper>;
	Sleeper *prev, *next;
public:
	uintptr_t deadline;

	inline bool operator < (uintptr_t time) const {
		return (intptr_t)(deadline - time) < 0;
	}

	inline bool operator < (const Sleeper& other) const {
		return *this < other.deadline;
	}
};

#endif /* SLEEPER_H_ */
