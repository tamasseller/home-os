/*
 * MutexPriorityInversion.cpp
 *
 *  Created on: 2017.06.08.
 *      Author: tooma
 */

#include "CommonTestUtils.h"

#include "algorithm/Str.h"

namespace {
	typedef OsRt4 Os;
	typedef Os::Mutex Mutex;
	Mutex m;
	char trace[32] = {'\0',}, *tp = trace;

	struct T1: public TestTask<T1, Os> {
		void run() {
			*tp++ = 'a';
			Os::sleep(2);
			*tp++ = 'f';
			m.lock();
			*tp++ = 'h';
			m.unlock();
			*tp++ = 'i';
		}
	} t1;

	struct T2: public TestTask<T2, Os> {
		bool done = false;
		void run() {
			*tp++ = 'b';
			Os::sleep(1);
			*tp++ = 'e';
			CommonTestUtils::busyWork(CommonTestUtils::getIterations(80));
			*tp++ = 'j';
		}
	} t2;

	struct T3: public TestTask<T3, Os> {
		void run() {
			*tp++ = 'c';
			m.lock();
			*tp++ = 'd';
			CommonTestUtils::busyWork(CommonTestUtils::getIterations(10));
			*tp++ = 'g';
			m.unlock();
			*tp++ = 'k';
		}
	} t3;
}

TEST(MutexPriorityInversion)
{
	m.init();
	t1.start(0);
	t2.start(1);
	t3.start(2);

	CommonTestUtils::start<Os>();

	CHECK(pet::Str::ncmp(trace, "abcdefghijk", sizeof(trace)) == 0);
}
