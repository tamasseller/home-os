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

template<class Os>
class Test {
	struct T1: public TestTask<T1, Os> { bool done = false; bool started = false; bool run(); };
	struct T2: public TestTask<T2, Os> { bool done = false; bool started = false; bool run(); };
	struct T3: public TestTask<T3, Os> { bool done = false; bool started = false; bool run(); };

	static T1 t1;
	static T1 t2;
	static T2 t3;
	static T3 t4;
	static T3 t5;

public:
	static void work() {
		t1.start((uint8_t)0);
		t2.start((uint8_t)0);
		t3.start((uint8_t)1);
		t4.start((uint8_t)2);
		t5.start((uint8_t)2);

		CommonTestUtils::start<Os>();

		CHECK(!t1.error && t1.done);
		CHECK(!t2.error && t2.done);
		CHECK(!t3.error && t3.done);
		CHECK(!t4.error && t4.done);
		CHECK(!t5.error && t5.done);
	}
};

template<class Os>
typename Test<Os>::T1 Test<Os>::t1;

template<class Os>
typename Test<Os>::T1 Test<Os>::t2;

template<class Os>
typename Test<Os>::T2 Test<Os>::t3;

template<class Os>
typename Test<Os>::T3 Test<Os>::t4;

template<class Os>
typename Test<Os>::T3 Test<Os>::t5;

template<class Os>
bool Test<Os>::T1::run() {
	started = true;
	CommonTestUtils::busyWorkMs(30);
	done = true;

	if(!(t1.started && t2.started && !t3.started && !t4.started && !t5.started))
		return T1::TestTask::bad;

	return T1::TestTask::ok;
}

template<class Os>
bool Test<Os>::T2::run() {
	started = true;
	CommonTestUtils::busyWorkMs(30);
	done = true;

	if(!(t1.done && t2.done && !t4.started && !t5.started))
		return T2::TestTask::bad;

	return T2::TestTask::ok;
}

template<class Os>
bool Test<Os>::T3::run() {
	started = true;
	CommonTestUtils::busyWorkMs(30);
	done = true;

	if(!(t1.done && t2.done && t3.done && t4.started && t5.started))
		return T3::TestTask::bad;

	return T3::TestTask::ok;
}

}

TEST(PolicyRoundRobinPrio) {
	Test<OsRrPrio>::work();
}

TEST(PolicyRealtimePrio) {
	Test<OsRt4>::work();
}
