/*
 * OsTestBase.h
 *
 *  Created on: 2017.05.29.
 *      Author: tooma
 */

#ifndef OSTESTBASE_H_
#define OSTESTBASE_H_

#include "Os.h"

#include "1test/Test.h"

void startOs();

template<uintptr_t size, uintptr_t count>
struct TestStackPool {
	static uintptr_t stacks[size * count];
	static uintptr_t *current;

	static void* get() {
		void *ret = current;
		current += size / sizeof(uintptr_t);
		return ret;
	}

	static void clear() {
		current = stacks;
	}
};

template<uintptr_t size, uintptr_t count>
uintptr_t TestStackPool<size, count>::stacks[];

template<uintptr_t size, uintptr_t count>
uintptr_t *TestStackPool<size, count>::current = TestStackPool<size, count>::stacks;

typedef TestStackPool<testStackSize, testStackCount> StackPool;

template<class Child>
struct TestTask: Os::Task<Child>
{
	template<class... Args>
	void start(Args... args) {
		Os::Task<Child>::start(StackPool::get(), testStackSize, args...);
	}
};

#endif /* OSTESTBASE_H_ */
