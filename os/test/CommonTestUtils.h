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

#include "policy/RoundRobinPrioPolicy.h"
#include "policy/RealtimePolicy.h"

using OsRr = Scheduler<
		SchedulerOptions::HardwareProfile<ProfileCortexM0>,
		SchedulerOptions::SchedulingPolicy<RoundRobinPolicy>,
		SchedulerOptions::EnableAssert<true>
>;

using OsRrPrio = Scheduler<
		SchedulerOptions::HardwareProfile<ProfileCortexM0>,
		SchedulerOptions::SchedulingPolicy<RoundRobinPrioPolicy>,
		SchedulerOptions::EnableAssert<true>
>;

using OsRt4 = Scheduler<
		SchedulerOptions::HardwareProfile<ProfileCortexM0>,
		SchedulerOptions::SchedulingPolicy<RealtimePolicy<4>::Policy>,
		SchedulerOptions::EnableAssert<true>
>;


template<uintptr_t size, uintptr_t count>
struct TestStackPool {
	static uintptr_t stacks[size * count / sizeof(uintptr_t)];
	static uintptr_t *current;

	static void* get() {
		void *ret = current;
		current += size / sizeof(uintptr_t);
		return ret;
	}

	static void clear() {
		for(int i=0; i<sizeof(stacks)/sizeof(stacks[0]); i++)
			stacks[i] = 0x1badc0de;
		current = stacks;
	}
};

template<uintptr_t size, uintptr_t count>
uintptr_t TestStackPool<size, count>::stacks[];

template<uintptr_t size, uintptr_t count>
uintptr_t *TestStackPool<size, count>::current = TestStackPool<size, count>::stacks;

typedef TestStackPool<testStackSize, testStackCount> StackPool;

template<class Child, class Os=OsRr>
struct TestTask: Os::Task
{
	template<class... Args>
	void start(Args... args) {
		Os::Task::template start<Child, &Child::run>(StackPool::get(), testStackSize, args...);
	}
};


class CommonTestUtils {
	static uint64_t iterationsPerMs;
public:
	static uintptr_t startParam;

	static void busyWork(uint64_t iterations);
	static void calibrate();
	static uint64_t getIterations(uintptr_t ms);

	template<class Os=OsRr>
	static void start() {
		Os::start(startParam);
	}
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

	inline void reset() {
		for(int i = sizeof(data)/sizeof(data[0]) - 1; i >= 0; i--)
			data[i] = 0;

		error = false;
	}
};

#endif /* COMMONTESTUTILS_H_ */
