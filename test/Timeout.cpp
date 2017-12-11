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

#include "meta/Sequence.h"

using Os=OsRr;

namespace {
	volatile uint32_t counter;

	using Sleeper = OsInternalTester<Os>::Sleeper;

	template<bool periodic>
	struct Timeout: OsInternalTester<Os>::Timeout {
		static void increment(::Sleeper* s) {
			counter++;

			if(periodic)
				static_cast<Timeout*>(s)->extend(5);
		}

		Timeout(): OsInternalTester<Os>::Timeout(&increment) {}
	};

	Timeout<false> timeouts[10];
	Timeout<true> periodicTimeouts[3];
}

TEST_GROUP(Timeout) {};

TEST(Timeout, Simple)
{
	struct Task: public TestTask<Task> {
		bool run() {
			for(unsigned int i=0; i < sizeof(timeouts)/sizeof(timeouts[0]); i++)
				timeouts[i].start(10 + 3 * i);
			while(counter != 10);

			auto exit = Os::getTick();
			if(exit < 37 || 40 < exit)
				return bad;

			return ok;
		}
	} task;

	task.start();
	counter = 0;
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(Timeout, Interrupt)
{
	struct Task: public TestTask<Task> {
		static void isr() {
			for(unsigned int i=0; i < sizeof(timeouts)/sizeof(timeouts[0]); i++)
				timeouts[i].start(10 + 3 * i);

			CommonTestUtils::registerIrq(nullptr);
		}

		bool run() {
			CommonTestUtils::registerIrq(isr);
			while(counter != 10);

			auto exit = (int)Os::getTick();
			if(exit < 37 || 40 < exit)
				return bad;

			return ok;
		}
	} task;

	task.start();
	counter = 0;
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(Timeout, Cancel)
{
	struct Task: public TestTask<Task> {
		Os::TickType exit;
		bool run() {
			for(unsigned int i=0; i < sizeof(timeouts)/sizeof(timeouts[0]); i++)
				timeouts[i].start(10);

			for(unsigned int i=0; i < sizeof(timeouts)/sizeof(timeouts[0]); i++)
				timeouts[i].cancel();

			Os::sleep(100);

			return ok;
		}
	} task;

	task.start();
	counter = 0;
	CommonTestUtils::start();
	CHECK(!counter);
}

TEST(Timeout, Periodic)
{
	struct Task: public TestTask<Task> {
		Os::TickType exit;
		bool run() {
			for(unsigned int i=0; i < sizeof(periodicTimeouts)/sizeof(periodicTimeouts[0]); i++)
				periodicTimeouts[i].start(3);

			Os::sleep(15);

			for(unsigned int i=0; i < sizeof(periodicTimeouts)/sizeof(periodicTimeouts[0]); i++)
				periodicTimeouts[i].cancel();

			Os::sleep(15);

			return ok;
		}
	} task;

	task.start();
	counter = 0;
	CommonTestUtils::start();
	CHECK(counter == 9);
}
