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

TEST_GROUP(SemaphorePassaround) {
	typedef typename OsRr::BinarySemaphore Semaphore;

	struct Task: public TestTask<Task> {
		SharedData<16> &data;
		Semaphore &sPend, &sSend;
		int counter = 0;
		bool to;
		bool run() {
			for(int i = 0; i < UINT16_MAX/30; i++) {
				if(to) {
					if(!sPend.wait(100))
						return bad;
				}else
					sPend.wait();

				data.update();
				sSend.notifyFromTask();
				counter++;
			}

			return ok;
		}

		Task(SharedData<16> &data, Semaphore &sPend, Semaphore &sSend): data(data), sPend(sPend), sSend(sSend) {}

		void start(bool to) {
			counter = 0;
			this->to = to;
			TestTask::start();
		}
	};
};

TEST(SemaphorePassaround, WithoutTimeout)
{
	Semaphore s1, s2, s3;
	SharedData<16> data;
	Task t1(data, s1, s2), t2(data, s2, s3), t3(data, s3, s1);

	data.reset();
	t1.start(false);
	t2.start(false);
	t3.start(false);
	s1.init(true);
	s2.init(false);
	s3.init(false);

	CommonTestUtils::start();

	CHECK(!t1.error && t1.counter == UINT16_MAX/30);
	CHECK(!t2.error && t2.counter == UINT16_MAX/30);
	CHECK(!t3.error && t3.counter == UINT16_MAX/30);
	CHECK(data.check(UINT16_MAX/30*3));
}


TEST(SemaphorePassaround, WithTimeout)
{
	Semaphore s1, s2, s3;
	SharedData<16> data;
	Task t1(data, s1, s2), t2(data, s2, s3), t3(data, s3, s1);

	data.reset();
	t1.start(true);
	t2.start(true);
	t3.start(true);
	s1.init(true);
	s2.init(false);
	s3.init(false);

	CommonTestUtils::start();

	CHECK(!t1.error && t1.counter == UINT16_MAX/30);
	CHECK(!t2.error && t2.counter == UINT16_MAX/30);
	CHECK(!t3.error && t3.counter == UINT16_MAX/30);
	CHECK(data.check(UINT16_MAX/30*3));
}

