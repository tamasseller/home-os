/*
 * MutexSingleBusy.cpp
 *
 *  Created on: 2017.05.29.
 *      Author: tooma
 */

#include "common/CommonTestUtils.h"

namespace {
	static OsRr::Mutex mutex;
	static SharedData<16> data;

	struct T1: public TestTask<T1> {
		void run() {
			for(int i = 0; i < UINT16_MAX - 20; i++) {
				mutex.lock();
				data.update();
				mutex.unlock();
			}
		}
	} t1;

	struct T2: public TestTask<T2> {
		void run() {
			for(int i=0; i<10; i++) {
				mutex.lock();
				data.update();
				OsRr::sleep(5);
				data.update();
				mutex.unlock();
				OsRr::sleep(5);
			}
		}
	} t2;
}

TEST(SingleMutexBusy) {
	t1.start();
	t2.start();
	mutex.init();

	CommonTestUtils::start();

	CHECK(data.check(UINT16_MAX));
}
