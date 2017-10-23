/*
 * Select.cpp
 *
 *  Created on: 2017.06.05.
 *      Author: tooma
 */

#include "CommonTestUtils.h"

namespace {
	typedef typename OsRr::BinarySemaphore Semaphore;
	static Semaphore s12, s23, s31;
	static Semaphore s13, s32, s21;

	struct Task: public TestTask<Task> {
		Semaphore &sPendForward, &sSendForward;
		Semaphore &sPendReverse, &sSendReverse;
		int counter = 0;
		void run() {
			for(int i = 0; i < UINT16_MAX/3; i++) {
				if(OsRr::select(&sPendForward, &sPendReverse) == &sPendForward)
					sSendForward.notify();
				else
					sSendReverse.notify();

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

	OsRr::BinarySemaphore semTo;
	bool error = false;
	struct TaskTo: TestTask<TaskTo> {
		void run() {

			if(OsRr::selectTimeout(0, &semTo) != &semTo)
				error = true;

			if(OsRr::selectTimeout(0, &semTo) != nullptr)
				error = true;

			for(int i=0; i<10; i++)
				if(OsRr::selectTimeout(10, &semTo) != nullptr)
					error = true;
		}
	} tTo;
}

TEST(SelectPassaround)
{
	s12.init(true);
	s23.init(false);
	s31.init(false);

	s32.init(true);
	s21.init(false);
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
