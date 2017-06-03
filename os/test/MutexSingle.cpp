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
		void run() {
			for(int i = 0; i < UINT16_MAX/2; i++) {
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
