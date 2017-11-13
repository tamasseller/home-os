/*******************************************************************************
 *
 * Copyright (c) 2017 Seller Tamás. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *******************************************************************************/

#ifndef POLICY_H_
#define POLICY_H_

#include "Scheduler.h"

namespace home {

template<class... Args>
class Scheduler<Args...>::Policy: Blocker, PolicyBase {

	/*
	 * These two methods are never called during normal operation, so
	 * LCOV_EXCL_START is placed here to exclude them from coverage analysis
	 */

	virtual void canceled(Blockable*, Blocker*) override final {
		assert(false, ErrorStrings::policyBlockerUsage);
	}

	virtual void timedOut(Blockable*) override final {
		assert(false, ErrorStrings::policyBlockerUsage);
	}

	virtual Blocker* getTakeable(Task*) override final {
		assert(false, ErrorStrings::policyBlockerUsage);
		return nullptr;
	}

	virtual void block(Blockable*) override final {
		assert(false, ErrorStrings::policyBlockerUsage);
	}

	virtual uintptr_t acquire(Task*) override final {
		assert(false, ErrorStrings::policyBlockerUsage);
		return 0;
	}

	virtual bool release(uintptr_t arg) override final {
		assert(false, ErrorStrings::policyBlockerUsage);
		return false;
	}

	/*
	 * From here on, the rest should be check for test coverage, so
	 * LCOV_EXCL_STOP is placed here.
	 */

	virtual void priorityChanged(Blockable* b, typename PolicyBase::Priority old) override final {
		assert(b->getTask() == b, ErrorStrings::policyNonTask);
		PolicyBase::priorityChanged(static_cast<Task*>(b), old);
	}

public:
	using Priority = typename PolicyBase::Priority;

	Task* peekNext() {
		return static_cast<Task*>(PolicyBase::peekNext());
	}

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

}

#endif /* POLICY_H_ */
