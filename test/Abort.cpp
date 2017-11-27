/*******************************************************************************
 *
 * Copyright (c) 2017 Seller Tamás. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *******************************************************************************/

#include "common/CommonTestUtils.h"

using Os=OsRr;

namespace {
	Os::Mutex abortMutex;
	uint32_t abortCounter, limit;

	struct Task: public TestTask<Task> {
		bool run() {
			while(true) {
				abortMutex.lock();

				if(++abortCounter == limit)
					Os::abort("Done");

				abortMutex.unlock();
			}

			return ok;
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
