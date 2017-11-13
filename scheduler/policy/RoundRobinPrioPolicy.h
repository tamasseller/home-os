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

#ifndef ROUNDROBINPRIOPOLICY_H_
#define ROUNDROBINPRIOPOLICY_H_

#include "data/DoubleList.h"

namespace home {

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

	Task* popNext() {
		// For some odd reason the explicit nullcheck is required (gcc 6.3.0).
		if(Storage* element = tasks.popLowest())
			return static_cast<Task*>(element);

		return nullptr;
	}

	Task* peekNext() {
		// For some odd reason the explicit nullcheck is required (gcc 6.3.0).
		if(Storage* element = tasks.lowest())
			return static_cast<Task*>(element);

		return nullptr;
	}

	inline static void initialize(Task* task, Priority prio) {
		task->staticPriority = prio.staticPriority;
	}

	void priorityChanged(Task* task, Priority old) {
		tasks.remove(task);
		tasks.add(task);
	}
};

}

#endif /* ROUNDROBINPRIOPOLICY_H_ */
