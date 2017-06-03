/*
 * SemaphoreLock.cpp
 *
 *  Created on: 2017.06.02.
 *      Author: tooma
 */

#include "CommonTestUtils.h"

namespace {
	static typename OsRr::BinarySemaphore sem;
	static SharedData<16> data;

	struct Task: public TestTask<Task> {
		void run() {
			for(int i = 0; i < UINT16_MAX/5; i++) {
				sem.wait();
				data.update();
				sem.notify();
			}
		}
	} t1, t2, t3, t4, t5;
}

TEST(SemaphoreLock) {
	t1.start();
	t2.start();
	t3.start();
	t4.start();
	t5.start();
	sem.init(true);

	CommonTestUtils::start();

	CHECK(data.check(UINT16_MAX/5*5));
}


