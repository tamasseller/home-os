/*
 * SemaphorePrio.cpp
 *
 *  Created on: 2017.06.06.
 *      Author: tooma
 */

#include "common/CommonTestUtils.h"

namespace {
	static typename OsRrPrio::BinarySemaphore sem;

	bool error = false;

	struct TWaiter: public TestTask<TWaiter, OsRrPrio> {
		int counter = 0;
		void run() {
			for(int i = 0; i < 15000; i++) {
				sem.wait();
				counter++;
			}
		}
	} t1, t2;

	struct TSender: public TestTask<TSender, OsRrPrio> {
		int counter = 0;
		void run() {
			for(int i = 0; i < 10000; i++) {
				sem.notifyFromTask();
				counter++;
			}
		}
	} t3, t4, t5;
}

TEST(SemaphorePrio) {
	t1.start(0);
	t2.start(0);
	t3.start(1);
	t4.start(1);
	t5.start(1);
	sem.init(false);

	CommonTestUtils::start<OsRrPrio>();

	CHECK(!error);
	CHECK(t1.counter == 15000);
	CHECK(t2.counter == 15000);
	CHECK(t3.counter == 10000);
	CHECK(t4.counter == 10000);
	CHECK(t5.counter == 10000);
}

