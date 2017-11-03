/*
 * Abort.cpp
 *
 *  Created on: 2017.11.03.
 *      Author: tooma
 */

#include "common/CommonTestUtils.h"

using Os=OsRr;

namespace asd{
	Os::Mutex m;
	uint32_t counter, limit;

	struct Task: public TestTask<Task> {
		void run() {
			while(true) {
				m.lock();

				if(++counter == limit)
					Os::abort("Done");

				m.unlock();
			}
		}
	} t[4];
}


TEST(Abort)
{
	using namespace asd;
	counter = 0;

	for(int i = 0; i < 4; i++) {
		limit = (i+1) * 65536;

		m.init();

		for(int j = 0; j < 4; j++)
			t[j].start();

		CommonTestUtils::start("Done");
		CHECK(counter == limit);
		StackPool::clear();
	}
}


