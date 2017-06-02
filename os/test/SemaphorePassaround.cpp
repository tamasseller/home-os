/*
 * SemaphorePassaround.cpp
 *
 *  Created on: 2017.06.02.
 *      Author: tooma
 */

#include "CommonTestUtils.h"

namespace SemaphorePassaround {
	typedef typename Os::BinarySemaphore Semaphore;
	static Semaphore s1, s2, s3;
	static SharedData<16> data;
	bool error = false;

	struct Task: public TestTask<Task> {
		Semaphore &sPend, &sSend;
		int counter = 0;
		bool to;
		void run() {
			for(int i = 0; i < UINT16_MAX/3; i++) {
				if(to) {
					if(!sPend.wait(100))
						error = true;
				}else
					sPend.wait();

				data.update();
				sSend.notify();
				counter++;
			}
		}

		Task(Semaphore &sPend, Semaphore &sSend): sPend(sPend), sSend(sSend) {}

		void start(bool to) {
			counter = 0;
			this->to = to;
			TestTask::start();
		}
	} t1(s1, s2), t2(s2, s3), t3(s3, s1);
}

TEST(SemaphorePassaround)
{
	using namespace SemaphorePassaround;
	t1.start(false);
	t2.start(false);
	t3.start(false);
	s1.init(true);
	s2.init(false);
	s3.init(false);

	CommonTestUtils::start();

	CHECK(data.check(UINT16_MAX/3*3));
	CHECK(t1.counter == UINT16_MAX/3);
	CHECK(t2.counter == UINT16_MAX/3);
	CHECK(t3.counter == UINT16_MAX/3);
}


TEST(SemaphorePassaroundWithTimeout)
{
	using namespace SemaphorePassaround;
	t1.start(true);
	t2.start(true);
	t3.start(true);
	s1.init(true);
	s2.init(false);
	s3.init(false);

	CommonTestUtils::start();

	CHECK(data.check(UINT16_MAX/3*3));
	CHECK(t1.counter == UINT16_MAX/3);
	CHECK(t2.counter == UINT16_MAX/3);
	CHECK(t3.counter == UINT16_MAX/3);
}

