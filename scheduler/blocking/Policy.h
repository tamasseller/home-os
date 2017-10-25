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

	/*
	 * These two methods are never called during normal operation, so
	 * LCOV_EXCL_START is placed here to exclude them from coverage analysis
	 */

	virtual void remove(Blockable*, Blocker*) override final {
		assert(false, "Only priority change can be handled through the Blocker interface of the Policy wrapper");
	}

	virtual Blocker* getTakeable(Task*) override final {
		assert(false, "Only priority change can be handled through the Blocker interface of the Policy wrapper");
		return nullptr;
	}

	virtual void block(Blockable*) override final {
		assert(false, "Only priority change can be handled through the Blocker interface of the Policy wrapper");
	}

	virtual uintptr_t acquire(Task*) override final {
		assert(false, "Only priority change can be handled through the Blocker interface of the Policy wrapper");
		return 0;
	}

	virtual bool release(uintptr_t arg) override final {
		assert(false, "Only priority change can be handled through the Blocker interface of the Policy wrapper");
		return false;
	}

	/*
	 * From here on, the rest should be check for test coverage, so
	 * LCOV_EXCL_STOP is placed here.
	 */

	virtual void priorityChanged(Blockable* b, Priority old) override final {
		assert(b->getTask() == b, "Only tasks can be handled by the policy container");
		PolicyBase::priorityChanged(static_cast<Task*>(b), old);
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
