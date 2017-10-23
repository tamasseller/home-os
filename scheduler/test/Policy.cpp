/*
 * Policy.cpp
 *
 *  Created on: 2017.06.14.
 *      Author: tooma
 */

#include "CommonTestUtils.h"

template<template<class, class> class Policy>
struct TestData {
	class Task;
	typedef Policy<Task, Task> P;
	class Task: public P::Priority {
		friend pet::DoubleList<Task>;
		Task* next, *prev;
	} t1, t2, t3, t4;

	P p;

	void sanity() {
		CHECK(!p.peekNext());
		p.addRunnable(&t1);
		CHECK(p.peekNext() == &t1);
		CHECK(p.popNext() == &t1);
		CHECK(p.peekNext() == nullptr);
	}
};

TEST(RoundRobinPolicy) {
	typedef TestData<RoundRobinPolicy> Data;
	Data data;

	Data::P::initialize(&data.t1);
	Data::P::initialize(&data.t2);
	Data::P::initialize(&data.t3);
	Data::P::initialize(&data.t4);

	data.sanity();

	data.p.addRunnable(&data.t2);
	data.p.addRunnable(&data.t1);
	CHECK(data.p.popNext() == &data.t2);
	CHECK(data.p.popNext() == &data.t1);
	CHECK(data.p.popNext() == nullptr);
}

TEST(RoundRobinPrioPolicy) {
	typedef TestData<RoundRobinPrioPolicy> Data;
	Data data;

	Data::P::initialize(&data.t1, 1);
	Data::P::initialize(&data.t2, 2);
	Data::P::initialize(&data.t3, 2);
	Data::P::initialize(&data.t4, 3);

	data.sanity();

	data.p.addRunnable(&data.t2);
	data.p.addRunnable(&data.t1);
	CHECK(data.p.popNext() == &data.t1);
	CHECK(data.p.popNext() == &data.t2);
	CHECK(data.p.popNext() == nullptr);
}

TEST(RealtimePolicy) {
	typedef TestData<RealtimePolicy<4>::Policy> Data;
	Data data;

	Data::P::initialize(&data.t1, 0);
	Data::P::initialize(&data.t2, 1);
	Data::P::initialize(&data.t3, 2);
	Data::P::initialize(&data.t4, 3);

	data.sanity();

	data.p.addRunnable(&data.t2);
	data.p.addRunnable(&data.t1);
	CHECK(data.p.popNext() == &data.t1);
	CHECK(data.p.popNext() == &data.t2);
	CHECK(data.p.popNext() == nullptr);
}
