/*
 * MutexVsSelect.cpp
 *
 *  Created on: 2017.06.17.
 *      Author: tooma
 */

#include "CommonTestUtils.h"

#include "algorithm/Str.h"

namespace {
	static OsRrPrio::Mutex mutex;
	static OsRrPrio::BinarySemaphore sem;
	char trace[32] = {'\0',}, * volatile tp = trace;

	struct T1: public TestTask<T1, OsRrPrio> {
		volatile int counter = -1;
		void run() {
			*tp++ = 'a';
			OsRrPrio::sleep(10);
			*tp++ = 'f';
			mutex.lock();
			*tp++ = 'i';
			mutex.unlock();
			*tp++ = 'j';
		}
	} t1;

	struct T2: public TestTask<T2, OsRrPrio> {
		volatile int counter = -1;
		void run() {
			*tp++ = 'b';
			OsRrPrio::select(&sem);
			*tp++ = 'm';
		}
	} t2;

	struct T3: public TestTask<T3, OsRrPrio> {
		volatile int counter = -1;
		void run() {
			*tp++ = 'c';
			mutex.lock();
			*tp++ = 'd';
			OsRrPrio::select(&sem);
			*tp++ = 'h';
			mutex.unlock();
			*tp++ = 'k';
		}
	} t3;

	struct T4: public TestTask<T4, OsRrPrio> {
		volatile int counter = -1;
		void run() {
			*tp++ = 'e';
			OsRrPrio::sleep(20);
			*tp++ = 'g';
			sem.notify();
			*tp++ = 'l';
			sem.notify();
			*tp++ = 'n';
		}
	} t4;
}

TEST(MutexVsSelect) {
	t1.start(0);
	t2.start(1);
	t3.start(2);
	t4.start(3);
	mutex.init();
	sem.init(false);

	CommonTestUtils::start<OsRrPrio>();

	CHECK(pet::Str::ncmp(trace, "abcdefghijklmn", sizeof(trace)) == 0);
}
