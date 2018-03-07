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

	template<class Child>
	struct Common: public TestTask<Common<Child>, OsRrPrio> {
		int counter = 0;

		bool started() {
			return counter != 0;
		}

		bool finished() {
			return counter == 201;
		}

		bool run() {
			for(int i = 0; i < 200; i++) {
				sem.wait();
				counter++;
			}

			counter++;
			return ((Child*)this)->check();
		}

	};

	struct T1: Common<T1> { bool check(); } t1, t2;
	struct T2: Common<T2> { bool check(); } t3, t4;

	bool T1::check() {
		bool highStarted = t1.started() && t2.started();
		bool highFinished = t1.finished() && t2.finished();
		bool lowNotStarted = !t3.started() || !t4.started();

		if(!highStarted)
			return T1::Common::TestTask::bad;

		if(!highFinished)
			if(!lowNotStarted)
				return T1::Common::TestTask::bad;

		return T1::Common::TestTask::ok;
	}

	bool T2::check() {
		bool highFinished = t1.finished() && t2.finished();
		bool lowStarted = t3.started() && t4.started();

		if(!(highFinished && lowStarted))
			return T1::Common::TestTask::bad;

		return T1::Common::TestTask::ok;
	}

	int irqCounter = 0;

	void irq() {
		sem.notifyFromInterrupt();
		sem.notifyFromInterrupt();

		if(++irqCounter == 400) {
			CommonTestUtils::registerIrq(nullptr);
		}
	}
}

TEST(SemaphoreFromIrq) {
	t1.start(uint8_t(0));
	t2.start(uint8_t(0));
	t3.start(uint8_t(1));
	t4.start(uint8_t(1));
	sem.init(false);

	CommonTestUtils::registerIrq(irq);
	CommonTestUtils::start<OsRrPrio>();

	CHECK(!t1.error && t1.finished());
	CHECK(!t2.error && t2.finished());
	CHECK(!t3.error && t3.finished());
	CHECK(!t4.error && t4.finished());
	CHECK(irqCounter == 400);
}
