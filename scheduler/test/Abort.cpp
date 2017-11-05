/*
 * Abort.cpp
 *
 *  Created on: 2017.11.03.
 *      Author: tooma
 */

#include "common/CommonTestUtils.h"

using Os=OsRr;

namespace {
	Os::Mutex abortMutex;
	uint32_t abortCounter, limit;

	struct Task: public TestTask<Task> {
		void run() {
			while(true) {
				abortMutex.lock();

				if(++abortCounter == limit)
					Os::abort("Done");

				abortMutex.unlock();
			}
		}
	} abortTasks[4];
}


TEST(Abort)
{
	abortCounter = 0;

	for(int i = 0; i < 16; i++) {
		limit = (i+1) * 256;

		abortMutex.init();

		for(size_t j = 0; j < sizeof(abortTasks)/sizeof(abortTasks[0]); j++)
			abortTasks[j].start();

		CommonTestUtils::start("Done");
		CHECK(abortCounter == limit);
		StackPool::clear();
	}
}
