/*
 * SemaphoreTimeout.cpp
 *
 *  Created on: 2017.06.03.
 *      Author: tooma
 */

#include "CommonTestUtils.h"

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
