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
		bool isRunnable;
	public:
		inline bool operator <(const Priority &other) const {
			return staticPriority < other.staticPriority;
		}

		inline void operator =(const Priority &other) {
			staticPriority = other.staticPriority;
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
		task->isRunnable = true;
	}

	Task* popNext() {
		if(Storage* element = tasks.popLowest()) {
			static_cast<Task*>(element)->isRunnable = false;
			return static_cast<Task*>(element);
		}

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

	void inheritPriority(Task* target, Task* source)
	{
		if(target->isRunnable)
			tasks.remove(target);

		target->staticPriority = source->staticPriority;

		if(target->isRunnable)
			tasks.add(target);
	}
};




#endif /* ROUNDROBINPRIOPOLICY_H_ */
