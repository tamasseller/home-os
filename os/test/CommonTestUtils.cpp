/*
 * CommonTestUtils.cpp
 *
 *  Created on: 2017.05.30.
 *      Author: tooma
 */

#include "CommonTestUtils.h"

uint64_t CommonTestUtils::iterationsPerMs;
uintptr_t CommonTestUtils::startParam;

__attribute__((noinline)) // XXX
void CommonTestUtils::busyWork(uint64_t iterations)
{
	volatile uint64_t x = iterations;
	while(x--);
}

void CommonTestUtils::calibrate()
{
	uint64_t trial = 10000;

	while(1) {
		auto startTicks = OsRr::getTick();
		busyWork(trial);
		auto endTicks = OsRr::getTick();
		auto time = endTicks - startTicks;

		if(time > 16) {
			iterationsPerMs = trial / time;
			break;
		} else
			trial *= 2;
	}
}

uint64_t CommonTestUtils ::getIterations(uintptr_t ms)
{
	return iterationsPerMs * ms;
}


void CommonTestUtils::start()
{
	OsRr::start(startParam);
}
