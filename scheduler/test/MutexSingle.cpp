/*
 * MutexSingle.cpp
 *
 *  Created on: 2017.05.29.
 *      Author: tooma
 */

#include "common/CommonTestUtils.h"

namespace {
	static OsRr::Mutex mutex;
	static SharedData<16> data;
	static constexpr auto nTasks = 3;

	struct Task: public TestTask<Task> {
		volatile int counter = -1;
		void run() {
			for(counter = 0; counter < UINT16_MAX/nTasks; counter++) {
				mutex.lock();
				mutex.lock(); 		// recursive relocking
				data.update();
				mutex.unlock();
				mutex.unlock();
			}
		}
	} t[nTasks];
}

TEST(SingleMutex) {
	for(auto i = 0; i<nTasks; i++)
		t[i].start();

	mutex.init();

	CommonTestUtils::start();

	CHECK(data.check(UINT16_MAX/nTasks*nTasks));
}
