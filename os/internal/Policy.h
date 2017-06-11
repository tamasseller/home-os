/*
 * Policy.h
 *
 *  Created on: 2017.06.11.
 *      Author: tooma
 */

#ifndef POLICY_H_
#define POLICY_H_

template<class... Args>
class Scheduler<Args...>::Policy: Blocker, PolicyBase {
	virtual void remove(Task*) {
		// assert(false);
	}

	virtual void waken(Task*, Waitable*) {
		// assert(false);
	}

public:
	using Priority = typename PolicyTemplate<Task, Blockable>::Priority;

	using PolicyBase::addRunnable;
	using PolicyBase::popNext;
	using PolicyBase::peekNext;
	using PolicyBase::initialize;

	using PolicyBase::inheritPriority; // TODO remove using and use through Blocker interface
};


#endif /* POLICY_H_ */
