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

	struct T1: public TestTask<T1> {
		void run() {
			for(int i = 0; i < UINT16_MAX - 20; i++) {
				mutex.lock();
				data.update();
				mutex.unlock();
			}
		}
	} t1;

	struct T2: public TestTask<T2> {
		void run() {
			for(int i=0; i<10; i++) {
				mutex.lock();
				data.update();
				OsRr::sleep(5);
				data.update();
				mutex.unlock();
				OsRr::sleep(5);
			}
		}
	} t2;
}

TEST(SingleMutexBusy) {
	t1.start();
	t2.start();
	mutex.init();

	CommonTestUtils::start();

	CHECK(data.check(UINT16_MAX));
}
