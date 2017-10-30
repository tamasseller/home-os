/*
 * CommonTestUtils.cpp
 *
 *  Created on: 2017.05.30.
 *      Author: tooma
 */

#include "CommonTestUtils.h"

uint64_t CommonTestUtils::iterationsPerMs;
uintptr_t CommonTestUtils::startParam;
void (*CommonTestUtils::registerIrq)(void (*)());

__attribute__((noinline)) // XXX
void CommonTestUtils::busyWorker(uint64_t iterations)
{
	volatile uint64_t x = iterations;
	while(x--);
}
