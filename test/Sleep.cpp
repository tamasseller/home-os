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

TEST(Sleep) {
	constexpr auto expectedRunTime = 105;
	constexpr auto allowedErrorPercent = 1;

	constexpr auto minRuntime = expectedRunTime * (100 - allowedErrorPercent) / 100;
	constexpr auto maxRuntime = expectedRunTime * (100 + allowedErrorPercent) / 100;

	struct Task: public TestTask<Task> {
		typename OsRr::TickType taskDiff;

		unsigned int time;
		inline Task(unsigned int time): time(time) {}

		bool run() {
			auto taskStart = OsRr::getTick();

			for(auto i = 0u; i < (expectedRunTime+time-1)/time; i++) {
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

	Task t[] = {Task(3), Task(3), Task(5), Task(5), Task(7)};

	for(auto i = 0u; i<sizeof(t)/sizeof(t[0]); i++)
		t[i].start();

	CommonTestUtils::start();

	for(auto i = 0u; i<sizeof(t)/sizeof(t[0]); i++)
		CHECK(!t[i].error && minRuntime < t[i].taskDiff && t[i].taskDiff < maxRuntime);
}
