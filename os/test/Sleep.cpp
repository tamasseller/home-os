/*
 * Sleep.cpp
 *
 *  Created on: 2017.06.03.
 *      Author: tooma
 */

#include "CommonTestUtils.h"

namespace {
	constexpr auto expectedRunTime = 500;
	constexpr auto allowedErrorPercent = 10;

	constexpr auto minRuntime = expectedRunTime * (100 - allowedErrorPercent) / 100;
	constexpr auto maxRuntime = expectedRunTime * (100 + allowedErrorPercent) / 100;

	bool error = false;

	template<int time>
	struct Task: public TestTask<Task<time>> {
		typename OsRr::TickType taskDiff;

		void run() {
			auto taskStart = OsRr::getTick();

			for(int i = 0; i < (expectedRunTime+time-1)/time; i++) {
				auto start = OsRr::getTick();

				OsRr::sleep(time);

				auto diff = OsRr::getTick() - start;

				if(diff < time || time + 1 < diff)
					error = true;
			}

			taskDiff = OsRr::getTick() - taskStart;
		}
	};

	Task<42> t1, t2, t3;
	Task<57> t4, t5;
	Task<103> t6;
}

TEST(Sleep) {
	t1.start();
	t2.start();
	t3.start();
	t4.start();
	t5.start();
	t6.start();

	CommonTestUtils::start();

	CHECK(minRuntime < t1.taskDiff && t1.taskDiff < maxRuntime);
	CHECK(minRuntime < t2.taskDiff && t2.taskDiff < maxRuntime);
	CHECK(minRuntime < t3.taskDiff && t3.taskDiff < maxRuntime);
	CHECK(minRuntime < t4.taskDiff && t4.taskDiff < maxRuntime);
	CHECK(minRuntime < t5.taskDiff && t5.taskDiff < maxRuntime);
	CHECK(!error);
}
