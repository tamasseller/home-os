/*
 * Temporal.cpp
 *
 *  Created on: 2018.03.07.
 *      Author: tooma
 */

#include "frontend/Temporal.h"

#include "1test/Test.h"

using namespace home;

TEST_GROUP(Temporal) {};

TEST(Temporal, ConvertFiner)
{
	Seconds s(1);
	MilliSecs ms(s);
	MicroSecs us(s);
	NanoSecs ns(s);

	CHECK((uint32_t)ms == 1000);
	CHECK((uint32_t)us == 1000 * 1000);
	CHECK((uint32_t)ns == 1000 * 1000 * 1000);
}

TEST(Temporal, ConvertCoarser)
{
	NanoSecs ns(1414213562);
	MicroSecs us(ns);
	MilliSecs ms(us);
	Seconds s(ms);

	CHECK((uint32_t)us == 1414214);
	CHECK((uint32_t)ms == 1415);
	CHECK((uint32_t)s == 2);
}

TEST(Temporal, Additive)
{
	NanoSecs ns1(1001), ns2(1);
	MicroSecs us(1);

	CHECK((uint32_t)(ns1 + us) == 2001);
	CHECK((uint32_t)(us + ns1) == 3);

	CHECK((uint32_t)(us - ns2) == 0);
	CHECK((uint32_t)(ns1 - us) == 1);
}

TEST(Temporal, Scalar)
{
	MicroSecs us(345);

	CHECK((uint32_t)(us * 1u) == 345);
	CHECK((uint32_t)(us * 3u) == 1035);
	CHECK((uint32_t)(us * 5u) == 1725);

	CHECK((uint32_t)(us / 1u) == 345);
	CHECK((uint32_t)(us / 3u) == 115);
	CHECK((uint32_t)(us / 5u) == 69);
}

TEST(Temporal, CompareSimple)
{
	MicroSecs ms(1500), ms1(1000);
	MilliSecs s1(1), s2(2);

	CHECK(s1 == s1);
	CHECK(s1 <= s1);
	CHECK(s1 >= s1);

	CHECK(s1 != s2);
	CHECK(s1 < s2);
	CHECK(s2 > s1);

	CHECK(!(s1 == ms));
	CHECK(!(ms == s2));

	CHECK(s1 != ms);
	CHECK(ms != s2);

	CHECK(s1 < ms);
	CHECK(ms < s2);

	CHECK(s1 <= ms);
	CHECK(ms <= s2);

	CHECK(ms > s1);
	CHECK(s2 > ms);

	CHECK(ms >= s1);
	CHECK(s2 >= ms);

	CHECK(s1 == ms1);
	CHECK(s2 == ms1 * 2u);
}

TEST(Temporal, InstantModify)
{
	Time<MilliSecs> t(0);

	t += MilliSecs(2);

	CHECK(uint32_t(t) == 2);

	t += MicroSecs(1000);

	CHECK(uint32_t(t) == 3);
	CHECK(t == Time<MilliSecs>(2) + MicroSecs(1000));

	t += MicroSecs(123);

	CHECK(uint32_t(t) == 4);

	t += Seconds(1);

	CHECK(uint32_t(t) == 1004);

	t -= MicroSecs(123);

	CHECK(uint32_t(t) == 1003);
	CHECK(t == Time<MilliSecs>(1003));
	CHECK(t != Time<MilliSecs>(0x1d1075));
	CHECK(t == Time<MilliSecs>(1004) - MicroSecs(123));


	t -= MicroSecs(1000);

	CHECK(uint32_t(t) == 1002);
}

TEST(Temporal, InstantCompare)
{
	Time<MilliSecs> t1(0xfffffff0);
	Time<MilliSecs> t2(0xfffffff8);
	Time<MilliSecs> t3(0);
	Time<MilliSecs> t4(8);
	Time<MilliSecs> t5(16);

	CHECK(t1 < t2);
	CHECK(t2 < t3);
	CHECK(t3 < t4);
	CHECK(t4 < t5);

	CHECK(t1 <= t2);
	CHECK(t2 <= t3);
	CHECK(t3 <= t4);
	CHECK(t4 <= t5);

	CHECK(t5 > t4);
	CHECK(t4 > t3);
	CHECK(t3 > t2);
	CHECK(t2 > t1);

	CHECK(t5 >= t4);
	CHECK(t4 >= t3);
	CHECK(t3 >= t2);
	CHECK(t2 >= t1);
}
