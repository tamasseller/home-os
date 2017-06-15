/*
 * SemaphoreFromIrq.cpp
 *
 *  Created on: 2017.06.11.
 *      Author: tooma
 */

#include "CommonTestUtils.h"

namespace {
	static typename OsRrPrio::CountingSemaphore sem;

	bool error = false;

	template<class Child>
	struct Common: public TestTask<Common<Child>, OsRrPrio> {
		int counter = 0;
		bool started = false, finished = false;

		void run() {
			for(int i = 0; i < 200; i++) {
				sem.wait();
				started = true;
				counter++;
			}

			finished = true;
			((Child*)this)->check();
		}

	};

	struct T1: Common<T1> { void check(); } t1, t2;
	struct T2: Common<T2> { void check(); } t3, t4;

	void T1::check() {
		if(!(t1.started && t2.started && (!t3.started || !t4.started)))
			error = true;
	}

	void T2::check() {
		if(!(t1.finished && t2.finished && t3.started && t4.started))
			error = true;
	}

	int irqCounter = 0;

	void irq() {
		if(irqCounter++ == 400) {
			CommonTestUtils::registerIrq(nullptr);
		}

		sem.notify();
		sem.notify();
	}
}

TEST(SemaphoreFromIrq) {
	t1.start(0);
	t2.start(0);
	t3.start(1);
	t4.start(1);
	sem.init(false);

	CommonTestUtils::registerIrq(irq);
	CommonTestUtils::start<OsRrPrio>();

	CHECK(!error);
	CHECK(t1.counter == 200);
	CHECK(t2.counter == 200);
	CHECK(t3.counter == 200);
	CHECK(t4.counter == 200);
	CHECK(irqCounter == 400);
}
