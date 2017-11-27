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

TEST(SingleMutexBusy)
{
	OsRr::Mutex mutex;
	SharedData<16> data;

	struct T1: public TestTask<T1> {
		OsRr::Mutex &mutex;
		SharedData<16> &data;
		inline T1(OsRr::Mutex &mutex, SharedData<16> &data): mutex(mutex), data(data) {}

		bool run() {
			for(int i = 0; i < 4096 - 20; i++) {
				mutex.lock();
				data.update();
				mutex.unlock();
			}

			return ok;
		}
	} t1(mutex, data);

	struct T2: public TestTask<T2> {
		OsRr::Mutex &mutex;
		SharedData<16> &data;
		inline T2(OsRr::Mutex &mutex, SharedData<16> &data): mutex(mutex), data(data) {}

		bool run() {
			for(int i=0; i<10; i++) {
				mutex.lock();
				data.update();
				OsRr::sleep(5);
				data.update();
				mutex.unlock();
				OsRr::sleep(5);
			}

			return ok;
		}
	} t2(mutex, data);

	t1.start();
	t2.start();
	mutex.init();

	CommonTestUtils::start();

	CHECK(data.check(4096));
}
