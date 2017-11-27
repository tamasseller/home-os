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
	constexpr auto expectedRunTime = 105;
	constexpr auto allowedErrorPercent = 1;

	constexpr auto minRuntime = expectedRunTime * (100 - allowedErrorPercent) / 100;
	constexpr auto maxRuntime = expectedRunTime * (100 + allowedErrorPercent) / 100;

	template<int time>
	struct Task: public TestTask<Task<time>> {
		typename OsRr::TickType taskDiff;

		bool run() {
			auto taskStart = OsRr::getTick();

			for(int i = 0; i < (expectedRunTime+time-1)/time; i++) {
				auto start = OsRr::getTick();

				OsRr::sleep(time);

				auto diff = OsRr::getTick() - start;

				if(diff < time || time + 1 < diff)
					return Task::TestTask::bad;
			}

			taskDiff = OsRr::getTick() - taskStart;

			return Task::TestTask::ok;
		}
	};

	Task<3> t1, t2;
	Task<5> t3, t4;
	Task<7> t5;
}

TEST(Sleep) {
	t1.start();
	t2.start();
	t3.start();
	t4.start();
	t5.start();

	CommonTestUtils::start();

	CHECK(!t1.error && minRuntime < t1.taskDiff && t1.taskDiff < maxRuntime);
	CHECK(!t2.error && minRuntime < t2.taskDiff && t2.taskDiff < maxRuntime);
	CHECK(!t3.error && minRuntime < t3.taskDiff && t3.taskDiff < maxRuntime);
	CHECK(!t4.error && minRuntime < t4.taskDiff && t4.taskDiff < maxRuntime);
	CHECK(!t5.error && minRuntime < t5.taskDiff && t5.taskDiff < maxRuntime);
}
