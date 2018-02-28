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

#ifndef ATOMICLIST_H_
#define ATOMICLIST_H_

#include "Scheduler.h"

#include "Atomic.h"
#include "internal/AtomicUtils.h"

namespace home {

template<class... Args>
class Scheduler<Args...>::SharedAtomicList
{
public:
	class Element {
		friend class SharedAtomicList;
		Atomic<uintptr_t> arg = 0;
		Element* next;
	};

	class Reader {
		Element* current;
		friend class SharedAtomicList;

		inline Reader(Element* current): current(current) {}
	public:
		Element* pop(uintptr_t& arg) {
			if(Element* ret = current) {
				/*
				 * Update the iterator first, because with clearing the
				 * argument, the ownership of the next field is transfered
				 * back to the writer side.
				 */
				current = current->next;

				arg = AtomicUtils::reset(ret->arg);

				return ret;
			}

			return nullptr;
		}
	};

private:
	Atomic<Element*> first;

public:
	template<class Combiner, class... CombinerArgs>
	inline void push(Element* element, Combiner&& combiner, CombinerArgs... args) {
		if(!element->arg(combiner, args...)) {
			first([&](Element* old, Element* &result) {
				element->next = old;
				result = element;
				return true;
			});
		}
	}

	inline Reader read()
	{
		/*
		 * Take over the current list atomically.
		 */
		Element* current = AtomicUtils::reset(first);

		/*
		 * The list is in reverse order, in order to enable sane
		 * behavior when used as a queue, the list is reversed.
		 */

		/// _prev_ is the previous element in the original order.
		Element* prev = nullptr;

		/*
		 * _current_ moves through the list in the original order.
		 */
		while(current) {
			/*
			 * Get the original next, this is going to be _current_
			 * in the next iteration.
			 */
			Element* oldNext = current->next;

			/*
			 * Set the next pointer of the current to be the other
			 * neighbor, thus reversing the order of elements.
			 */
			current->next = prev;

			/*
			 * Set up state for the next iteration:
			 *   - _prev_ is going to be the _current_ of this run
			 *   - _current_ is going to be oldNext, the next in
			 *     the original ordering.
			 */
			prev = current;
			current = oldNext;
		}

		/*
		 * At this point _current_ is null and _prev_ holds
		 * the last element encountered before the end of the
		 * list in original order, which is the first in the
		 * reversed order.
		 */
		return prev;
	}
};

}

#endif /* ATOMICLIST_H_ */
