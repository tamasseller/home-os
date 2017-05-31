/*
 * RoundRobinPolicy.h
 *
 *  Created on: 2017.05.24.
 *      Author: tooma
 */

#ifndef ROUNDROBINPOLICY_H_
#define ROUNDROBINPOLICY_H_

#include "data/DoubleList.h"

template<class Blockable>
struct RoundRobinPolicy {
	struct Priority{
		inline bool operator <(const Priority &other) const {
			return false;
		}
	};

	pet::DoubleList<Blockable> tasks;

	void addRunnable(Blockable* task) {
		tasks.fastAddBack(task);
	}

	void removeRunnable(Blockable* task) {
		tasks.fastRemove(task);
	}

	Blockable* popNext() {
		return tasks.popFront();
	}

	Blockable* peekNext() {
		return tasks.front();
	}
};

#endif /* ROUNDROBINPOLICY_H_ */
