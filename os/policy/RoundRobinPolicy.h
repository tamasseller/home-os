/*
 * RoundRobinPolicy.h
 *
 *  Created on: 2017.05.24.
 *      Author: tooma
 */

#ifndef ROUNDROBINPOLICY_H_
#define ROUNDROBINPOLICY_H_

#include "data/DoubleList.h"

template<class Waiter>
struct RoundRobinPolicy {
	struct Priority{
		inline bool operator <(const Priority &other) const {
			return false;
		}
	};

	pet::DoubleList<Waiter> tasks;

	void addRunnable(Waiter* task) {
		tasks.fastAddBack(task);
	}

	void removeRunnable(Waiter* task) {
		tasks.fastRemove(task);
	}

	Waiter* popNext() {
		return tasks.popFront();
	}

	Waiter* peekNext() {
		return tasks.front();
	}
};

#endif /* ROUNDROBINPOLICY_H_ */
