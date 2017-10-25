/*
 * Sleeper.h
 *
 *  Created on: 2017.06.11.
 *      Author: tooma
 */

#ifndef SLEEPER_H_
#define SLEEPER_H_

template<class... Args>
class Scheduler<Args...>::Sleeper {
	static constexpr Sleeper *invalid() {return (Sleeper *)0xffffffff;}

	friend pet::DoubleList<Sleeper>;
	Sleeper *prev = invalid(), *next;

public:
	uintptr_t deadline;

	inline bool isSleeping() {
		return prev != invalid();
	}

	inline void invalidate() {
		prev = invalid();
	}
};

#endif /* SLEEPER_H_ */
