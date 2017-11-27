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

#include "algorithm/Str.h"

namespace {
	typedef OsRt4::Mutex Mutex;
	Mutex m1, m2;
	char trace[32] = {'\0',}, * volatile tp = trace;

	struct T0: public TestTask<T0, OsRt4> {
		bool run() {
			*tp++ = 'a';
			OsRt4::sleep(30);
			*tp++ = 'h';
			m2.lock();
			*tp++ = 'k';
			m2.unlock();

			return ok;
		}
	} t0;

	struct T1: public TestTask<T1, OsRt4> {
		bool run() {
			*tp++ = 'b';
			OsRt4::sleep(20);
			*tp++ = 'g';
			m2.lock();
			m1.lock();
			*tp++ = 'j';
			m2.unlock();
			*tp++ = 'l';
			m1.unlock();

			return ok;
		}
	} t1;

	struct T2: public TestTask<T2, OsRt4> {
		bool done = false;
		bool run() {
			*tp++ = 'c';
			OsRt4::sleep(10);
			*tp++ = 'f';
			CommonTestUtils::busyWorkMs(50);
			*tp++ = 'm';

			return ok;
		}
	} t2;

	struct T3: public TestTask<T3, OsRt4> {
		bool run() {
			*tp++ = 'd';
			m1.lock();
			*tp++ = 'e';
			CommonTestUtils::busyWorkMs(40);
			*tp++ = 'i';
			m1.unlock();
			*tp++ = 'n';

			return ok;
		}
	} t3;
}

TEST(MutexPriorityInversion)
{
	t0.start((uint8_t)0);
	t1.start((uint8_t)1);
	t2.start((uint8_t)2);
	t3.start((uint8_t)3);
	m1.init();
	m2.init();

	CommonTestUtils::start<OsRt4>();

	CHECK(pet::Str::ncmp(trace, "abcdefghijklmn", sizeof(trace)) == 0);
}
