/*
 * MutexSingle.cpp
 *
 *  Created on: 2017.05.29.
 *      Author: tooma
 */

#include "CommonTestUtils.h"

namespace {
	static OsRr::Mutex mutex;
	static SharedData<16> data;

	struct Task: public TestTask<Task> {
		volatile int counter = -1;
		void run() {
			for(counter = 0; counter < UINT16_MAX/2; counter++) {
				mutex.lock();
				data.update();
				mutex.unlock();
			}
		}
	} t1, t2;
}

TEST(SingleMutex) {
	t1.start();
	t2.start();
	mutex.init();

	CommonTestUtils::start();

	CHECK(data.check(UINT16_MAX-1));
}
