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

	void addRunnable(Task* task) {
		Task** next = &first;
		while(*next)
			next = &((*next)->next);

		*next = task;
		task->next = nullptr;
	}

	void removeRunnable(Task* task) {
		Task** next = &first;
		while(*next) {
			if(*next == task) {
				*next = task->next;
				return;
			}
			next = &((*next)->next);
		}
	}

	Task* getNext() {
		Task* ret = first;
		first = first->next;
		return ret;
	}

};

#endif /* ROUNDROBINPOLICY_H_ */
