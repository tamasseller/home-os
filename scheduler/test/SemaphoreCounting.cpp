/*
 * SemaphoreCounting.cpp
 *
 *  Created on: 2017.06.15.
 *      Author: tooma
 */

#include "common/CommonTestUtils.h"

namespace {
	static typename OsRrPrio::CountingSemaphore sem;

	bool error = false;

	struct Task: public TestTask<Task, OsRrPrio> {
		int counter = 0;

		void run() {
			for(int i = 0; i < 150; i++) {
				sem.wait();
				counter++;
			}
		}
	} t1, t2;

	int irqCounter = 0;

	void irq() {
		sem.notifyFromInterrupt();
		sem.notifyFromInterrupt();
		sem.notifyFromInterrupt();

		if(++irqCounter == 100) {
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
	CHECK(t1.counter == 150);
	CHECK(t2.counter == 150);
	CHECK(irqCounter == 100);
}
