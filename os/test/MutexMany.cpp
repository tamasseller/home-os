/*
 * MutexMany.cpp
 *
 *  Created on: 2017.05.30.
 *      Author: tooma
 */

#include "CommonTestUtils.h"

namespace {
	Os::Mutex m1, m2, m3;
	SharedData<8> d1, d2, d3;

	struct Task: public TestTask<Task> {
		Os::Mutex &m1, &m2;
		SharedData<8> &d1, &d2;
		Task(SharedData<8> &d1, SharedData<8> &d2, Os::Mutex &m1, Os::Mutex &m2): d1(d1), d2(d2), m1(m1), m2(m2) {}

		void run() {
			for(int i = 0; i < UINT16_MAX/2; i++) {
				m1.lock();
				m2.lock();
				d1.update();
				d2.update();
				m2.unlock();
				m1.unlock();
			}
		}
	};
}

TEST(ManyMutex) {
	Task t1(d1, d2, m1, m2);
	Task t2(d1, d3, m1, m3);
	Task t3(d2, d3, m2, m3);
	m1.init();
	m2.init();
	m3.init();
	t1.start();
	t2.start();
	t3.start();

	CommonTestUtils::start();

	CHECK(d1.check(UINT16_MAX - 1));
	CHECK(d2.check(UINT16_MAX - 1));
	CHECK(d3.check(UINT16_MAX - 1));
}
