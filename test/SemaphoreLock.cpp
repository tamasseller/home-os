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
	static typename OsRr::BinarySemaphore sem;
	static SharedData<16> data;

	static constexpr auto nTasks = 4;

	struct Task: public TestTask<Task> {
		bool run() {
			for(int i = 0; i < 4096/nTasks; i++) {
				sem.wait();
				data.update();
				sem.notifyFromTask();
			}

			return Task::TestTask::ok;
		}
	} t[nTasks];
}

TEST(SemaphoreLock) {
	for(unsigned int i=0; i<sizeof(t)/sizeof(t[0]); i++)
		t[i].start();

	sem.init(true);

	CommonTestUtils::start();

	CHECK(data.check(4096/nTasks*nTasks));
}
