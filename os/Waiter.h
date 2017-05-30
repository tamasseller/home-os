/*
 * Waiter.h
 *
 *  Created on: 2017.05.30.
 *      Author: tooma
 */

#ifndef WAITER_H_
#define WAITER_H_

#include "Scheduler.h"

template<class Profile, template<class> class PolicyParam>
class Scheduler<Profile, PolicyParam>::Waiter: public Policy::Priority {
	static constexpr Waiter *invalid = (Waiter *)0xffffffff;

	friend pet::DoubleList<Waiter>;
	Waiter *prev = invalid, *next;
public:
	inline bool isWaiting() {
		return prev == invalid;
	}

	inline void invalidate() {
		prev = invalid;
	}
};

#endif /* WAITER_H_ */
