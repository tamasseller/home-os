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
class Scheduler<Args...>::Policy: Blocker {
	static constexpr uintptr_t maxLevels = sizeof(uintptr_t) * 8;
	static_assert(priorityLimit < maxLevels, "Too many priority levels (maximum allowed: native word size in bits)");

private:
	uintptr_t cache = 0;
	pet::DoubleList<Blockable> tasks[priorityLimit];

	bool findCurrentLevel(uintptr_t& ret) {
		ret = clz(cache);
		return ret < priorityLimit;
	}

	static inline uintptr_t mask(uintptr_t level) {
		return (uintptr_t)1 << (maxLevels - level - 1);
	}

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

	virtual void priorityChanged(Blockable* b, uint8_t old) override final {
		auto task = static_cast<Task*>(b);

		assert(task->getTask() == task, ErrorStrings::policyNonTask);

		tasks[old].remove(task);

		if(!tasks[old].front())
			cache &= ~mask(old);

		uintptr_t level = task->priority;
		tasks[level].fastAddBack(task);
		cache |= mask(level);
	}

public:
	Task* popNext() {
		uintptr_t level;

		if(!findCurrentLevel(level))
			return nullptr;

		Task* task = static_cast<Task*>(tasks[level].popFront());

		if(!tasks[level].front())
			cache &= ~mask(level);

		task->blockedBy = nullptr;
		return task;
	}

	Task* peekNext() {
		uintptr_t level;

		if(!findCurrentLevel(level))
			return nullptr;

		return static_cast<Task*>(tasks[level].front());
	}

	void addRunnable(Task* task) {
		uintptr_t level = task->priority;
		tasks[level].fastAddBack(task);
		cache |= mask(level);
		task->blockedBy = this;
	}

	inline static void initialize(Task* task, uint8_t prio = priorityLimit - 1) {
		task->priority = prio;
		task->blockedBy = nullptr;
	}
};

}

#endif /* POLICY_H_ */
