/*
 * TaskStart.cpp
 *
 *  Created on: 2017.06.15.
 *      Author: tooma
 */

#include "CommonTestUtils.h"

namespace {
	constexpr auto nTasks = 5;
	static bool error = false;

	template<int n> struct Task;

	template<>
	struct Task<0>: public TestTask<Task<0>> {
		OsRr::BinarySemaphore sem;
		bool done = false;

		void run() {
			CommonTestUtils::busyWork(CommonTestUtils::getIterations(10));
			done = true;
			sem.notify();
		}

		bool checkDone() {
			return done;
		}
	};

	template<int n>
	struct Task: public TestTask<Task<n>> {
		static Task<n-1> startee;
		OsRr::BinarySemaphore sem;

		bool done = false;
		void run() {
			startee.sem.init(false);
			startee.start();
			startee.sem.wait();

			if(!startee.done)
				error = true;

			done = true;
			sem.notify();
		}

		bool checkDone() {
			return done && startee.checkDone();
		}
	};

	template<int n>
	Task<n-1> Task<n>::startee;

	Task<nTasks> root;
}

TEST(TaskStart) {
	root.start();
	CommonTestUtils::start();
	CHECK(root.checkDone());
	CHECK(!error);
}
