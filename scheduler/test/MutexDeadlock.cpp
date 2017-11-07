/*
 * MutexDeadlock.cpp
 *
 *  Created on: 2017.11.07.
 *      Author: tooma
 */

#include "common/CommonTestUtils.h"

using Os=OsRr;

namespace {
	typename Os::Mutex m1, m2;

	struct Task1: public TestTask<Task1> {
		void run() {
			m1.lock();
			Os::yield();
			m2.lock();
			while(true);
		}
	} t1;

	struct Task2: public TestTask<Task2> {
		void run() {
			m2.lock();
			Os::yield();
			m1.lock();
			while(true);
		}
	} t2;

}

TEST(MutexDeadlock) {
	m1.init();
	m2.init();
	t1.start();
	t2.start();
	CommonTestUtils::start(Os::ErrorStrings::mutexDeadlock);
}
