/*
 * RealtimePolicy.h
 *
 *  Created on: 2017.06.05.
 *      Author: tooma
 */

#ifndef REALTIMEPOLICY_H_
#define REALTIMEPOLICY_H_

#include "ubiquitous/Compiler.h"

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
			return ret < maxLevels;
		}

		static inline uintptr_t mask(uintptr_t level) {
			return (1 << (maxLevels - level - 1));
		}

	public:
		class Priority{
			friend Policy;
			uintptr_t staticPriority;
		public:
			inline bool operator <(const Priority &other) const {
				return staticPriority < other.staticPriority;
			}

			inline Priority() {}
			inline Priority(uintptr_t staticPriority): staticPriority(staticPriority) {}
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

			Storage* element = tasks[level].popFront();
			// assert(element);

			if(!tasks[level].front())
				cache &= ~mask(level);

			return static_cast<Task*>(element);
		}

		Task* peekNext() {
			uintptr_t level;

			if(!findCurrentLevel(level))
				return nullptr;

			Storage* element = tasks[level].front();

			// assert(element);

			return static_cast<Task*>(element);
		}

		inline static void initialize(Task* task, Priority prio) {
			task->staticPriority = prio.staticPriority;
		}
};

};


#endif /* REALTIMEPOLICY_H_ */
