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

TEST(SingleMutex) {
	OsRr::Mutex mutex;
	SharedData<16> data;

	struct Task: public TestTask<Task> {
		volatile int counter = -1;
		OsRr::Mutex &mutex;
		SharedData<16> &data;

		inline Task(OsRr::Mutex &mutex, SharedData<16> &data): mutex(mutex), data(data) {}

		bool run() {
			for(counter = 0; counter < 1024; counter++) {
				mutex.lock();
				mutex.lock(); 		// recursive relocking
				data.update();
				mutex.unlock();
				mutex.unlock();
			}

			return ok;
		}
	} t[] = {Task(mutex, data), Task(mutex, data), Task(mutex, data)};

	static constexpr auto nTasks = sizeof(t)/sizeof(t[0]);

	for(auto i = 0u; i<nTasks; i++)
		t[i].start();

	mutex.init();

	CommonTestUtils::start();

	CHECK(data.check(1024*nTasks));
}
