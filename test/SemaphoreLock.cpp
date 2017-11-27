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

TEST(SemaphoreLock) {
	typename OsRr::BinarySemaphore sem;
	SharedData<16> data;

	struct Task: public TestTask<Task> {
		typename OsRr::BinarySemaphore &sem;
		SharedData<16> &data;

		inline Task(typename OsRr::BinarySemaphore &sem, SharedData<16> &data): sem(sem), data(data) {}


		bool run() {
			for(int i = 0; i < 1024; i++) {
				sem.wait();
				data.update();
				sem.notifyFromTask();
			}

			return Task::TestTask::ok;
		}
	} t[] = {Task(sem, data), Task(sem, data), Task(sem, data), Task(sem, data)};

	static constexpr auto nTasks = sizeof(t)/sizeof(t[0]);

	for(unsigned int i=0; i<nTasks ; i++)
		t[i].start();

	sem.init(true);

	CommonTestUtils::start();

	CHECK(data.check(1024*nTasks));
}
