/*
 * MutexPriorityInversion.cpp
 *
 *  Created on: 2017.06.08.
 *      Author: tooma
 */

#include "CommonTestUtils.h"

#include "algorithm/Str.h"

namespace {
	typedef OsRt4::Mutex Mutex;
	Mutex m1, m2;
	char trace[32] = {'\0',}, * volatile tp = trace;

	struct T0: public TestTask<T0, OsRt4> {
		void run() {
			*tp++ = 'a';
			OsRt4::sleep(3);
			*tp++ = 'h';
			m2.lock();
			*tp++ = 'k';
			m2.unlock();
		}
	} t0;

	struct T1: public TestTask<T1, OsRt4> {
		void run() {
			*tp++ = 'b';
			OsRt4::sleep(2);
			*tp++ = 'g';
			m2.lock();
			m1.lock();
			*tp++ = 'j';
			m2.unlock();
			*tp++ = 'l';
			m1.unlock();
		}
	} t1;

	struct T2: public TestTask<T2, OsRt4> {
		bool done = false;
		void run() {
			*tp++ = 'c';
			OsRt4::sleep(1);
			*tp++ = 'f';
			CommonTestUtils::busyWork(CommonTestUtils::getIterations(80));
			*tp++ = 'm';
		}
	} t2;

	struct T3: public TestTask<T3, OsRt4> {
		void run() {
			*tp++ = 'd';
			m1.lock();
			*tp++ = 'e';
			CommonTestUtils::busyWork(CommonTestUtils::getIterations(10));
			*tp++ = 'i';
			m1.unlock();
			*tp++ = 'n';
		}
	} t3;
}

TEST(MutexPriorityInversion)
{
	t0.start((uint8_t)0);
	t1.start((uint8_t)1);
	t2.start((uint8_t)2);
	t3.start((uint8_t)3);
	m1.init();
	m2.init();

	CommonTestUtils::start<OsRt4>();

	CHECK(pet::Str::ncmp(trace, "abcdefghijklmn", sizeof(trace)) == 0);
}
