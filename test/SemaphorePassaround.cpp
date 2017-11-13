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

namespace SemaphorePassaround {
	typedef typename OsRr::BinarySemaphore Semaphore;
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
				sSend.notifyFromTask();
				counter++;
			}
		}

		Task(Semaphore &sPend, Semaphore &sSend): sPend(sPend), sSend(sSend) {}

		void start(bool to) {
			counter = 0;
			this->to = to;
			TestTask::start();
		}
	};
}

namespace SemaphorePassaroundCtx1 {
	static SemaphorePassaround::Semaphore s1, s2, s3;
	SemaphorePassaround::Task t1(s1, s2), t2(s2, s3), t3(s3, s1);
}

namespace SemaphorePassaroundCtx2 {
	static SemaphorePassaround::Semaphore s1, s2, s3;
	SemaphorePassaround::Task t1(s1, s2), t2(s2, s3), t3(s3, s1);
}


TEST(SemaphorePassaround)
{
	using namespace SemaphorePassaround;
	using namespace SemaphorePassaroundCtx1;

	data.reset();
	t1.start(false);
	t2.start(false);
	t3.start(false);
	s1.init(true);
	s2.init(false);
	s3.init(false);

	CommonTestUtils::start();

	CHECK(t1.counter == UINT16_MAX/3);
	CHECK(t2.counter == UINT16_MAX/3);
	CHECK(t3.counter == UINT16_MAX/3);
	CHECK(data.check(UINT16_MAX/3*3));
}


TEST(SemaphorePassaroundWithTimeout)
{
	using namespace SemaphorePassaround;
	using namespace SemaphorePassaroundCtx2;

	data.reset();
	t1.start(true);
	t2.start(true);
	t3.start(true);
	s1.init(true);
	s2.init(false);
	s3.init(false);

	CommonTestUtils::start();

	CHECK(!error);

	CHECK(t1.counter == UINT16_MAX/3);
	CHECK(t2.counter == UINT16_MAX/3);
	CHECK(t3.counter == UINT16_MAX/3);
	CHECK(data.check(UINT16_MAX/3*3));
}

