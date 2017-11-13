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

#ifndef REALTIMEPOLICY_H_
#define REALTIMEPOLICY_H_

#include "ubiquitous/Compiler.h"

namespace home {

template<uintptr_t priorityLevels>
struct RealtimePolicy {
	static constexpr uintptr_t maxLevels = sizeof(uintptr_t) * 8;
	static_assert(priorityLevels < maxLevels, "Too many priority levels (maximum allowed: native word size in bits)");

	template<class Task, class Storage>
	class Policy {
		uintptr_t cache = 0;
		pet::DoubleList<Storage> tasks[priorityLevels];

		bool findCurrentLevel(uintptr_t& ret) {
			ret = clz(cache);
			return ret < priorityLevels;
		}

		static inline uintptr_t mask(uintptr_t level) {
			return (uintptr_t)1 << (maxLevels - level - 1);
		}

	public:
		class Priority{
			friend Policy;
			uint8_t staticPriority;
		public:
			inline bool operator <(const Priority &other) const {
				return staticPriority < other.staticPriority;
			}

			inline Priority(): staticPriority(0) {}
			inline Priority(uint8_t staticPriority): staticPriority(staticPriority) {}
		};


		void addRunnable(Task* task) {
			uintptr_t level = task->staticPriority;
			tasks[level].fastAddBack(task);

			cache |= mask(level);
		}

		Task* popNext() {
			uintptr_t level;

			if(!findCurrentLevel(level))
				return nullptr;

			Task* task = static_cast<Task*>(tasks[level].popFront());

			if(!tasks[level].front())
				cache &= ~mask(level);

			return task;
		}

		Task* peekNext() {
			uintptr_t level;

			if(!findCurrentLevel(level))
				return nullptr;

			return static_cast<Task*>(tasks[level].front());
		}

		void priorityChanged(Task* task, Priority old)
		{
			tasks[old.staticPriority].remove(task);

			if(!tasks[old.staticPriority].front())
				cache &= ~mask(old.staticPriority);

			uintptr_t level = task->staticPriority;
			tasks[level].fastAddBack(task);
			cache |= mask(level);
		}

		inline static void initialize(Task* task, Priority prio) {
			task->staticPriority = prio.staticPriority;
		}
};

};

}

#endif /* REALTIMEPOLICY_H_ */
