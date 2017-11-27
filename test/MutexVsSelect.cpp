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
	static OsRrPrio::Mutex mutex;
	static OsRrPrio::BinarySemaphore sem;
	char trace[32] = {'\0',}, * volatile tp = trace;

	struct T1: public TestTask<T1, OsRrPrio> {
		volatile int counter = -1;
		bool run() {
			*tp++ = 'a';
			OsRrPrio::sleep(10);
			*tp++ = 'f';
			mutex.lock();
			*tp++ = 'i';
			mutex.unlock();
			*tp++ = 'j';

			return ok;
		}
	} t1;

	struct T2: public TestTask<T2, OsRrPrio> {
		volatile int counter = -1;
		bool run() {
			*tp++ = 'b';
			OsRrPrio::select(&sem);
			*tp++ = 'm';

			return ok;
		}
	} t2;

	struct T3: public TestTask<T3, OsRrPrio> {
		volatile int counter = -1;
		bool run() {
			*tp++ = 'c';
			mutex.lock();
			*tp++ = 'd';
			OsRrPrio::select(&sem);
			*tp++ = 'h';
			mutex.unlock();
			*tp++ = 'k';

			return ok;
		}
	} t3;

	struct T4: public TestTask<T4, OsRrPrio> {
		volatile int counter = -1;
		bool run() {
			*tp++ = 'e';
			OsRrPrio::sleep(20);
			*tp++ = 'g';
			sem.notifyFromTask();
			*tp++ = 'l';
			sem.notifyFromTask();
			*tp++ = 'n';

			return ok;
		}
	} t4;
}

TEST(MutexVsSelect) {
	t1.start(0);
	t2.start(1);
	t3.start(2);
	t4.start(3);
	mutex.init();
	sem.init(false);

	CommonTestUtils::start<OsRrPrio>();

	CHECK(pet::Str::ncmp(trace, "abcdefghijklmn", sizeof(trace)) == 0);
}
