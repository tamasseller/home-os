/*
 * RoundRobinPolicy.h
 *
 *  Created on: 2017.05.24.
 *      Author: tooma
 */

#ifndef ROUNDROBINPOLICY_H_
#define ROUNDROBINPOLICY_H_

class RoundRobinPolicy {
	template<class, class> friend class Scheduler;

	class Task {
		friend RoundRobinPolicy;
		Task* next;
	};

	Task* first;

	bool addRunnable(Task* task) {
		Task** next = &first;
		while(*next)
			next = &((*next)->next);

		*next = task;
		task->next = nullptr;
	}

	Task* getNext() {
		Task* ret = first;
		first = first->next;
		return ret;
	}

	bool removeRunnable(Task*);
};

#endif /* ROUNDROBINPOLICY_H_ */
