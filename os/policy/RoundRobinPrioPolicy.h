/*
 * RoundRobinPrioPolicy.h
 *
 *  Created on: 2017.06.05.
 *      Author: tooma
 */

#ifndef ROUNDROBINPRIOPOLICY_H_
#define ROUNDROBINPRIOPOLICY_H_

#include "data/DoubleList.h"

template<class Task, class Storage>
class RoundRobinPrioPolicy {
public:
	class Priority{
		friend RoundRobinPrioPolicy;
		uintptr_t staticPriority;
	public:
		inline bool operator <(const Priority &other) const {
			return staticPriority < other.staticPriority;
		}

		inline Priority() {}
		inline Priority(uintptr_t staticPriority): staticPriority(staticPriority) {}
	};

private:
	static bool comparePriority(const Storage &a, const Storage &b) {
		return static_cast<const Task&>(a) < static_cast<const Task&>(b);
	}

	pet::OrderedDoubleList<Storage, &RoundRobinPrioPolicy::comparePriority> tasks;
public:

	void addRunnable(Task* task) {
		tasks.add(task);
	}

	void removeRunnable(Task* task) {
		tasks.fastRemove(task);
	}

	Task* popNext() {
		if(Storage* element = tasks.popLowest())
			return static_cast<Task*>(element);

		return nullptr;
	}

	Task* peekNext() {
		if(Storage* element = tasks.lowest())
			return static_cast<Task*>(element);

		return nullptr;
	}

	inline static void initialize(Task* task, Priority prio) {
		task->staticPriority = prio.staticPriority;
	}
};




#endif /* ROUNDROBINPRIOPOLICY_H_ */
