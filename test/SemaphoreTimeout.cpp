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

TEST(SemaphoreTimeout)
{
	OsRr::BinarySemaphore sem;


	struct T12: public TestTask<T12> {
		int counter = 0;
		OsRr::BinarySemaphore &sem;
		inline T12(OsRr::BinarySemaphore &sem): sem(sem) {}

		bool run() {
			for(int i = 0; i < 10; i++) {
				sem.wait();

				if(sem.wait(0)) return bad;
				if(sem.wait(10)) return bad;

				counter++;
				sem.notifyFromTask();
			}

			return ok;
		}
	} t1(sem), t2(sem);

	struct T3: public TestTask<T3> {
		int counter = 0;
		OsRr::BinarySemaphore &sem;
		inline T3(OsRr::BinarySemaphore &sem): sem(sem) {}

		bool run() {
			for(int i = 0; i < 100; i++) {
				while(!sem.wait(1));
				sem.notifyFromTask();
				counter++;
			}

			return ok;
		}
	} t3(sem);

	struct T4: public TestTask<T4> {
		int counter = 0;
		OsRr::BinarySemaphore &sem;
		inline T4(OsRr::BinarySemaphore &sem): sem(sem) {}

		bool run() {
			for(int i = 0; i < 100; i++) {
				while(!sem.wait(0));
				OsRr::sleep(1);
				sem.notifyFromTask();
				counter++;
			}

			return ok;
		}
	} t4(sem);

	t1.start();
	t2.start();
	t3.start();
	t4.start();
	sem.init(true);

	CommonTestUtils::start();

	CHECK(!t1.error && t1.counter == 10);
	CHECK(!t2.error && t2.counter == 10);
	CHECK(!t3.error && t3.counter == 100);
	CHECK(!t4.error && t4.counter == 100);
}
