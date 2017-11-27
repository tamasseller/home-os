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
	static typename OsRrPrio::CountingSemaphore sem;

	bool error = false;

	struct Task: public TestTask<Task, OsRrPrio> {
		int counter = 0;

		bool run() {
			for(int i = 0; i < 15; i++) {
				sem.wait();
				counter++;
			}

			return ok;
		}
	} t1, t2;

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
	t1.start(0);
	t2.start(0);
	sem.init(false);

	CommonTestUtils::registerIrq(irq);
	CommonTestUtils::start<OsRrPrio>();

	CHECK(!error);
	CHECK(t1.counter == 15);
	CHECK(t2.counter == 15);
	CHECK(irqCounter == 10);
}
