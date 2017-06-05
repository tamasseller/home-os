/*
 * RoundRobinPolicy.h
 *
 *  Created on: 2017.05.24.
 *      Author: tooma
 */

#ifndef ROUNDROBINPOLICY_H_
#define ROUNDROBINPOLICY_H_

#include "data/DoubleList.h"

template<class Task, class Storage>
struct RoundRobinPolicy {
	struct Priority{
		inline bool operator <(const Priority &other) const {
			return false;
		}
	};

	pet::DoubleList<Storage> tasks;

	void addRunnable(Task* task) {
		tasks.fastAddBack(task);
	}

	void removeRunnable(Task* task) {
		tasks.fastRemove(task);
	}

	Task* popNext() {
		if(Storage* element = tasks.popFront())
			return static_cast<Task*>(element);

		return nullptr;
	}

	Task* peekNext() {
		if(Storage* element = tasks.front())
			return static_cast<Task*>(element);

		return nullptr;
	}

	inline static void initialize(Task* task) {}
};

#endif /* ROUNDROBINPOLICY_H_ */
