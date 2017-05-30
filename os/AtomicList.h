/*
 * AtomicList.h
 *
 *  Created on: 2017.05.28.
 *      Author: tooma
 */

#ifndef ATOMICLIST_H_
#define ATOMICLIST_H_

#include "Scheduler.h"

template<class Profile, template<class> class PolicyParam>
class Scheduler<Profile, PolicyParam>::AtomicList
{
	template<class Value>
	using Atomic = typename Profile::template Atomic<Value>;
public:
	class Element {
		friend AtomicList;
		Atomic<uintptr_t> arg;
		Element* next;
	};

private:
	Atomic<Element*> first;

public:
	template<class Combiner, class... Args>
	inline void push(Element* element, Combiner&& combiner, Args... args) {
		if(!element->arg(combiner, args...)) {
			first([&](Element* old, Element* &result) {
				element->next = old;
				result = element;
				return true;
			});
		}
	}

	inline Element* pop(uintptr_t& arg)
	{
		Element* ret = first([&](Element* old, Element* &result) {
			if(!old)
				return false;

			result = old->next;
			return true;
		});

		if(ret) {
			arg = ret->arg([&](uintptr_t old, uintptr_t &result) {
				result = 0;
				return true;
			});
		}

		return ret;
	}
};

#endif /* ATOMICLIST_H_ */
