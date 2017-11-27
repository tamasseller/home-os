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
	constexpr auto nTasks = 2;
	int data[nTasks] = {0,};

	struct Task: public TestTask<Task> {
		bool run();
	} t[nTasks];

	bool Task::run() {
		uintptr_t selfIdx = this - t;
		for(int i = 0; i < 1000; i++) {
			data[selfIdx]++;
			OsRr::yield();
		}

		for(auto i = 0u; i < nTasks; i++)
			if(i != selfIdx && data[i])
				return Task::TestTask::ok;

		return Task::TestTask::bad;
	}
}

TEST(Yield) {
	for(int i = 0; i < nTasks; i++)
		t[i].start();

	CommonTestUtils::start();

	for(int i = 0; i < nTasks; i++)
		CHECK(data[i] == 1000);

	for(int i = 0; i < nTasks; i++)
		CHECK(!t[i].error);
}
