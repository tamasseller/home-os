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

namespace {
	static OsRr::Mutex mutex;
	static SharedData<16> data;
	static constexpr auto nTasks = 3;

	struct Task: public TestTask<Task> {
		volatile int counter = -1;
		bool run() {
			for(counter = 0; counter < 4096/nTasks; counter++) {
				mutex.lock();
				mutex.lock(); 		// recursive relocking
				data.update();
				mutex.unlock();
				mutex.unlock();
			}

			return ok;
		}
	} t[nTasks];
}

TEST(SingleMutex) {
	for(auto i = 0; i<nTasks; i++)
		t[i].start();

	mutex.init();

	CommonTestUtils::start();

	CHECK(data.check(4096/nTasks*nTasks));
}
