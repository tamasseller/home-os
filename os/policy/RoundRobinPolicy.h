/*
 * RoundRobinPolicy.h
 *
 *  Created on: 2017.05.24.
 *      Author: tooma
 */

#ifndef ROUNDROBINPOLICY_H_
#define ROUNDROBINPOLICY_H_

#include "data/DoubleList.h"

template<class FullTask>
class RoundRobinPolicy {
	template<class, template<class> class> friend class Scheduler;

	class Task {};

	pet::DoubleList<FullTask> tasks;

	void addRunnable(FullTask* task) {
		tasks.fastAddBack(task);
	}

	void removeRunnable(FullTask* task) {
		tasks.fastRemove(task);
	}

	bool isHigherPriority(FullTask* high, FullTask* low) {
		return false;
	}

	FullTask* popNext() {
		return tasks.popFront();
	}

	FullTask* peekNext() {
		return tasks.front();
	}
};

#endif /* ROUNDROBINPOLICY_H_ */
