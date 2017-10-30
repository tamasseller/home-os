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

		bool started() {
			return counter != 0;
		}

		bool finished() {
			return counter == 201;
		}

		void run() {
			for(int i = 0; i < 200; i++) {
				sem.wait();
				counter++;
			}

			counter++;
			((Child*)this)->check();
		}

	};

	struct T1: Common<T1> { void check(); } t1, t2;
	struct T2: Common<T2> { void check(); } t3, t4;

	void T1::check() {
		bool highStarted = t1.started() && t2.started();
		bool highFinished = t1.finished() && t2.finished();
		bool lowNotStarted = !t3.started() || !t4.started();

		if(!highStarted)
			error = true;

		if(!highFinished)
			if(!lowNotStarted)
				error = true;
	}

	void T2::check() {
		bool highFinished = t1.finished() && t2.finished();
		bool lowStarted = t3.started() && t4.started();

		if(!(highFinished && lowStarted))
			error = true;
	}

	int irqCounter = 0;

	void irq() {
		if(irqCounter++ == 400) {
			CommonTestUtils::registerIrq(nullptr);
		}

		sem.notifyFromInterrupt();
		sem.notifyFromInterrupt();
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
	CHECK(t1.finished());
	CHECK(t2.finished());
	CHECK(t3.finished());
	CHECK(t4.finished());
	CHECK(irqCounter == 400);
}
