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
	OsRr::Mutex m1, m2, m3;
	SharedData<8> d1, d2, d3;

	struct Task: public TestTask<Task> {
		OsRr::Mutex &m1, &m2;
		SharedData<8> &d1, &d2;
		Task(SharedData<8> &d1, SharedData<8> &d2, OsRr::Mutex &m1, OsRr::Mutex &m2): m1(m1), m2(m2), d1(d1), d2(d2)  {}

		void run() {
			for(int i = 0; i < UINT16_MAX/2; i++) {
				m1.lock();
				m2.lock();
				d1.update();
				d2.update();
				m2.unlock();
				m1.unlock();
			}
		}
	};

	Task t1(d1, d2, m1, m2);
	Task t2(d1, d3, m1, m3);
	Task t3(d2, d3, m2, m3);
}

TEST(ManyMutex) {
	m1.init();
	m2.init();
	m3.init();
	t1.start();
	t2.start();
	t3.start();

	CommonTestUtils::start();

	CHECK(d1.check(UINT16_MAX - 1));
	CHECK(d2.check(UINT16_MAX - 1));
	CHECK(d3.check(UINT16_MAX - 1));
}
