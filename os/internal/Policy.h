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
		assert(false,
			"Only priority change can be handled through the Blocker interface of the Policy wrapper");
	}

	virtual void waken(Task*, Waitable*) {
		assert(false,
			"Only priority change can be handled through the Blocker interface of the Policy wrapper");
	}

	virtual void priorityChanged(Task* task, Priority old) {
		PolicyBase::priorityChanged(task, old);
	}

public:
	using typename PolicyBase::Priority;

	using PolicyBase::peekNext;

	void addRunnable(Task* task) {
		PolicyBase::addRunnable(task);
		task->blockedBy = this;
	}

	Task* popNext() {
		if(Blockable* element = PolicyBase::popNext()) {
			Task* task = static_cast<Task*>(element);
			task->blockedBy = nullptr;
			return task;
		}

		return nullptr;
	}

	template<class... InitArgs>
	inline static void initialize(Task* task, InitArgs... initArgs) {
		PolicyBase::initialize(task, initArgs...);
		task->blockedBy = nullptr;
	}
};


#endif /* POLICY_H_ */
