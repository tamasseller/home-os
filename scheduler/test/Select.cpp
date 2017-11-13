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

using Os = OsRr;

namespace {
	typedef typename Os::BinarySemaphore Semaphore;
	static Semaphore s12, s23, s31;
	static Semaphore s13, s32, s21;

	struct Task: public TestTask<Task> {
		Semaphore &sPendForward, &sSendForward;
		Semaphore &sPendReverse, &sSendReverse;
		int counter = 0;
		void run() {
			for(int i = 0; i < UINT16_MAX/3; i++) {
				auto ret = Os::select(&sPendForward, &sPendReverse);
				if(ret == &sPendForward)
					sSendForward.notifyFromTask();
				else if(ret == &sPendReverse)
					sSendReverse.notifyFromTask();
				else
					Os::abort("Internal error detected during testing");

				counter++;
			}
		}

		Task(Semaphore &sPendForward, Semaphore &sSendForward, Semaphore &sPendReverse, Semaphore &sSendReverse):
			sPendForward(sPendForward), sSendForward(sSendForward), sPendReverse(sPendReverse), sSendReverse(sSendReverse) {}

		void start() {
			counter = 0;
			TestTask::start();
		}
	} 	t1(s31, s12, s21, s13),
		t2(s12, s23, s32, s21),
		t3(s23, s31, s13, s32);

	Os::BinarySemaphore semTo;
	bool error = false;
	struct TaskTo: TestTask<TaskTo> {
		void run() {

			if(Os::selectTimeout(0, &semTo) != &semTo)
				error = true;

			if(Os::selectTimeout(0, &semTo) != nullptr)
				error = true;

			for(int i=0; i<10; i++)
				if(Os::selectTimeout(10, &semTo) != nullptr)
					error = true;
		}
	} tTo;
}

TEST(SelectPassaround)
{
	s12.init(true);
	s23.init(false);
	s31.init(false);

	s32.init(false);
	s21.init(true);
	s13.init(false);

	t1.start();
	t2.start();
	t3.start();

	CommonTestUtils::start();

	CHECK(t1.counter == UINT16_MAX/3);
	CHECK(t2.counter == UINT16_MAX/3);
	CHECK(t3.counter == UINT16_MAX/3);
}

TEST(SelectTimeout)
{
	semTo.init(true);
	tTo.start();

	CommonTestUtils::start();

	CHECK(!error);
}
