/*
 * Yield.cpp
 *
 *  Created on: 2017.06.03.
 *      Author: tooma
 */

#include "CommonTestUtils.h"

namespace {
	constexpr auto nTasks = 3;
	int data[nTasks] = {0,};
	static bool error = false;

	struct Task: public TestTask<Task> {
		void run();
	} t[nTasks];

	void Task::run() {
		int selfIdx = this - t;
		for(int i = 0; i < 1000; i++) {
			data[selfIdx]++;
			OsRr::yield();
		}

		for(int i = 0; i < nTasks; i++)
			if(i != selfIdx && data[i])
				return;

		error = true;
	}
}

TEST(Yield) {
	for(int i = 0; i < nTasks; i++)
		t[i].start();

	CommonTestUtils::start();

	CHECK(!error);
}



