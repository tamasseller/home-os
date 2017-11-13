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
	constexpr auto nTasks = 5;
	static bool error = false;

	template<int n> struct Task;

	template<>
	struct Task<0>: public TestTask<Task<0>> {
		OsRr::BinarySemaphore sem;
		bool done = false;

		void run() {
			CommonTestUtils::busyWorkMs(10);
			done = true;
			sem.notifyFromTask();
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
			sem.notifyFromTask();
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
	root.sem.init(false);
	CommonTestUtils::start();
	CHECK(root.checkDone());
	CHECK(!error);
}
