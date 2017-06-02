/*
 * CommonTestUtils.h
 *
 *  Created on: 2017.05.30.
 *      Author: tooma
 */

#ifndef COMMONTESTUTILS_H_
#define COMMONTESTUTILS_H_

#include "1test/Test.h"

#include <stdint.h>

#include "Platform.h"
#include "Scheduler.h"

using Os = Scheduler<
		SchedulerOptions::HardwareProfile<ProfileCortexM0>,
		SchedulerOptions::SchedulingPolicy<RoundRobinPolicy>
>;

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
struct TestTask: Os::Task
{
	template<class... Args>
	void start(Args... args) {
		Os::Task::start<Child, &Child::run>(StackPool::get(), testStackSize, args...);
	}
};


class CommonTestUtils {
	static uint64_t iterationsPerMs;
public:
	static uintptr_t startParam;

	static void busyWork(uint64_t iterations);
	static void calibrate();
	static uint64_t getIterations(uintptr_t ms);
	static void start();
};

template<uintptr_t size>
class SharedData {
	uint16_t data[size];
	bool error = false;
public:
	inline void update() {
		for(int i = sizeof(data)/sizeof(data[0]) - 1; i >= 0; i--) {
			if(data[i] != data[0])
				error = true;

			data[i]++;
		}
	}

	inline bool check(uint16_t value) {
		for(int i = sizeof(data)/sizeof(data[0]) - 1; i >= 0; i--)
			if(data[i] != value)
				return false;

		return !error;
	}
};

#endif /* COMMONTESTUTILS_H_ */
