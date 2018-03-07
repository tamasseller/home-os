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
	typename OsRrPrio::CountingSemaphore sem;

	struct Task: public TestTask<Task, OsRrPrio> {
		int counter = 0;

		typename OsRrPrio::CountingSemaphore &sem;
		inline Task(typename OsRrPrio::CountingSemaphore &sem): sem(sem) {}

		bool run() {
			for(int i = 0; i < 15; i++) {
				sem.wait();
				counter++;
			}

			return ok;
		}
	} t1(sem), t2(sem);

	int irqCounter = 0;

	void irq() {
		sem.notifyFromInterrupt();
		sem.notifyFromInterrupt();
		sem.notifyFromInterrupt();

		if(++irqCounter == 10) {
			CommonTestUtils::registerIrq(nullptr);
		}
	}
}

TEST(SemaphoreCounting) {
	t1.start();
	t2.start();
	sem.init(false);

	CommonTestUtils::registerIrq(irq);
	CommonTestUtils::start<OsRrPrio>();

	CHECK(!t1.error && t1.counter == 15);
	CHECK(!t2.error && t2.counter == 15);
	CHECK(irqCounter == 10);
}
