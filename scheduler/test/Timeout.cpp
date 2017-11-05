/*
 * Timeout.cpp
 *
 *  Created on: 2017.11.05.
 *      Author: tooma
 */

#include "common/CommonTestUtils.h"

#include "meta/Sequence.h"

using Os=OsRr;

namespace {
	volatile uint32_t counter;

	using Timeout = OsInternalTester<Os>::Timeout;
	using Sleeper = OsInternalTester<Os>::Sleeper;

	static void increment(Sleeper*) {
		counter++;
	}

	template<class I> struct Helper;

	template<int... I>
	struct Helper<pet::Sequence<I...>> {
		typedef void(*f)(Sleeper*);
		static constexpr f dummy(int x) {return &increment;}

		static Timeout timeouts[sizeof...(I)];
	};

	template<int... I>
	Timeout Helper<pet::Sequence<I...>>::timeouts[sizeof...(I)] = {dummy(I)...};

	auto &timeouts = Helper<pet::sequence<0, 10>>::timeouts;
}

TEST_GROUP(Timeout) {};

TEST(Timeout, Simple)
{
	struct Task: public TestTask<Task> {
		Os::TickType exit;
		void run() {
			for(unsigned int i=0; i < sizeof(timeouts)/sizeof(timeouts[0]); i++)
				timeouts[i].start(10 + 3 * i);
			while(counter != 10);

			exit = (int)Os::getTick();
		}
	} task;

	task.start();
	counter = 0;
	CommonTestUtils::start();
	CHECK(37 <= task.exit && task.exit < 40);
}

TEST(Timeout, Interrupt)
{
	struct Task: public TestTask<Task> {
		Os::TickType exit;

		static void isr() {
			for(unsigned int i=0; i < sizeof(timeouts)/sizeof(timeouts[0]); i++)
				timeouts[i].start(10 + 3 * i);

			CommonTestUtils::registerIrq(nullptr);
		}

		void run() {

			CommonTestUtils::registerIrq(isr);
			while(counter != 10);
			exit = (int)Os::getTick();
		}
	} task;

	task.start();
	counter = 0;
	CommonTestUtils::start();
	CHECK(37 <= task.exit && task.exit < 40);
}

TEST(Timeout, Cancel)
{
	struct Task: public TestTask<Task> {
		Os::TickType exit;
		void run() {
			for(unsigned int i=0; i < sizeof(timeouts)/sizeof(timeouts[0]); i++)
				timeouts[i].start(100);

			for(unsigned int i=0; i < sizeof(timeouts)/sizeof(timeouts[0]); i++)
				timeouts[i].cancel();

			Os::sleep(1000);
		}
	} task;

	counter = 0;
	task.start();
	CommonTestUtils::start();
	CHECK(!counter);
}
