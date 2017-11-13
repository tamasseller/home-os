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

	bool error = false;

	struct T12: public TestTask<T12> {
		int counter = 0;
		void run() {
			for(int i = 0; i < 50; i++) {
				sem.wait();

				if(sem.wait(0)) {
					error = true;
					return;
				}

				if(sem.wait(10)) {
					error = true;
					return;
				}

				counter++;
				sem.notifyFromTask();
			}
		}
	} t1, t2;

	struct T3: public TestTask<T3> {
		int counter = 0;
		void run() {
			for(int i = 0; i < 500; i++) {
				while(!sem.wait(1));
				sem.notifyFromTask();
				counter++;
			}
		}
	} t3;

	struct T4: public TestTask<T4> {
		int counter = 0;
		void run() {
			for(int i = 0; i < 500; i++) {
				while(!sem.wait(0));
				OsRr::sleep(1);
				sem.notifyFromTask();
				counter++;
			}
		}
	} t4;
}

TEST(SemaphoreTimeout) {
	t1.start();
	t2.start();
	t3.start();
	t4.start();
	sem.init(true);

	CommonTestUtils::start();

	CHECK(!error);
	CHECK(t1.counter == 50);
	CHECK(t2.counter == 50);
	CHECK(t3.counter == 500);
	CHECK(t4.counter == 500);
}
