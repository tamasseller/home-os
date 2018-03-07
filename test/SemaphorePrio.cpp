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

TEST(SemaphorePrio) {
	OsRrPrio::BinarySemaphore sem;

	struct TWaiter: public TestTask<TWaiter, OsRrPrio> {
		int counter = 0;
		OsRrPrio::BinarySemaphore &sem;
		inline TWaiter(OsRrPrio::BinarySemaphore &sem): sem(sem) {}

		bool run() {
			for(int i = 0; i < 1500; i++) {
				sem.wait();
				counter++;
			}

			return ok;
		}
	} t1(sem), t2(sem);

	struct TSender: public TestTask<TSender, OsRrPrio> {
		int counter = 0;
		OsRrPrio::BinarySemaphore &sem;
		inline TSender(OsRrPrio::BinarySemaphore &sem): sem(sem) {}

		bool run() {
			for(int i = 0; i < 1000; i++) {
				sem.notifyFromTask();
				counter++;
			}

			return ok;
		}
	} t3(sem), t4(sem), t5(sem);

	t1.start(uint8_t(0));
	t2.start(uint8_t(0));
	t3.start(uint8_t(1));
	t4.start(uint8_t(1));
	t5.start(uint8_t(1));
	sem.init(false);

	CommonTestUtils::start<OsRrPrio>();

	CHECK(t1.counter == 1500);
	CHECK(t2.counter == 1500);
	CHECK(t3.counter == 1000);
	CHECK(t4.counter == 1000);
	CHECK(t5.counter == 1000);
}

