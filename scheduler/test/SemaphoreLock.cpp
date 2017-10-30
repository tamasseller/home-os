/*
 * SemaphoreLock.cpp
 *
 *  Created on: 2017.06.02.
 *      Author: tooma
 */

#include "common/CommonTestUtils.h"

namespace {
	static typename OsRr::BinarySemaphore sem;
	static SharedData<16> data;

	static constexpr auto nTasks = 4;

	struct Task: public TestTask<Task> {
		void run() {
			for(int i = 0; i < UINT16_MAX/nTasks; i++) {
				sem.wait();
				data.update();
				sem.notifyFromTask();
			}
		}
	} t[nTasks];
}

TEST(SemaphoreLock) {
	for(unsigned int i=0; i<sizeof(t)/sizeof(t[0]); i++)
		t[i].start();

	sem.init(true);

	CommonTestUtils::start();

	CHECK(data.check(UINT16_MAX/nTasks*nTasks));
}
