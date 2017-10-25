/*
 * SharedBlocker.h
 *
 *  Created on: 2017.10.25.
 *      Author: tooma
 */

#ifndef SHAREDBLOCKER_H_
#define SHAREDBLOCKER_H_

#include "Scheduler.h"

template<class... Args>
class Scheduler<Args...>::SharedBlocker: public Blocker
{
	virtual void priorityChanged(Blockable* b, Priority old) override {
		waiters.remove(b);
		waiters.add(b);
	}

	static bool comparePriority(const Blockable& a, const Blockable& b) {
			return firstPreemptsSecond(a.getTask(), b.getTask());
	}

protected:
	pet::OrderedDoubleList<Blockable, &SharedBlocker::comparePriority> waiters;

	virtual inline void block(Blockable* b) override {
		waiters.add(b);
	}
};

#endif /* SHAREDBLOCKER_H_ */
