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

#ifndef ROUNDROBINPOLICY_H_
#define ROUNDROBINPOLICY_H_

#include "data/DoubleList.h"

namespace home {

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

	void priorityChanged(Task* task, Priority old) {
	}

	inline static void initialize(Task* task) {}
};

}

#endif /* ROUNDROBINPOLICY_H_ */
