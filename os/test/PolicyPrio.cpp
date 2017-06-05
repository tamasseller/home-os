/*
 * PolicyPrio.cpp
 *
 *  Created on: 2017.06.05.
 *      Author: tooma
 */

#include "CommonTestUtils.h"

namespace {

template<class Os>
class Test {
	struct T1: public TestTask<T1, Os> { bool done = false; bool started = false; void run(); };
	struct T2: public TestTask<T2, Os> { bool done = false; bool started = false; void run(); };
	struct T3: public TestTask<T3, Os> { bool done = false; bool started = false; void run(); };

	static T1 t1;
	static T1 t2;
	static T2 t3;
	static T3 t4;
	static T3 t5;

	static bool error;

public:
	static void work() {
		t1.start(0);
		t2.start(0);
		t3.start(1);
		t4.start(2);
		t5.start(2);

		CommonTestUtils::start<Os>();

		CHECK(!error);

		CHECK(t1.done);
		CHECK(t2.done);
		CHECK(t3.done);
		CHECK(t4.done);
		CHECK(t5.done);
	}
};

template<class Os>
bool Test<Os>::error = false;


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
void Test<Os>::T1::run() {
	started = true;
	CommonTestUtils::busyWork(CommonTestUtils::getIterations(100));
	done = true;

	if(!(t1.started && t2.started && !t3.started && !t4.started && !t5.started))
		error = true;
}

template<class Os>
void Test<Os>::T2::run() {
	started = true;
	CommonTestUtils::busyWork(CommonTestUtils::getIterations(100));
	done = true;

	if(!(t1.done && t2.done && !t4.started && !t5.started))
		error = true;
}

template<class Os>
void Test<Os>::T3::run() {
	started = true;
	CommonTestUtils::busyWork(CommonTestUtils::getIterations(100));
	done = true;

	if(!(t1.done && t2.done && t3.done && t4.started && t5.started))
		error = true;
}

}

TEST(PolicyRoundRobinPrio) {
	Test<OsRrPrio>::work();
}

TEST(PolicyRealtimePrio) {
	Test<OsRt4>::work();
}
