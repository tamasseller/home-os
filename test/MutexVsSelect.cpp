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

TEST(MutexVsSelect)
{
	OsRrPrio::Mutex mutex;
	OsRrPrio::BinarySemaphore sem;
	char trace[32] = {'\0',}, * volatile tp = trace;

	struct T1: public TestTask<T1, OsRrPrio> {
		OsRrPrio::Mutex &mutex;
		char * volatile &tp;
		inline T1(OsRrPrio::Mutex &mutex, char * volatile &tp): mutex(mutex), tp(tp) {}

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
	} t1(mutex, tp);

	struct T2: public TestTask<T2, OsRrPrio> {
		OsRrPrio::BinarySemaphore &sem;
		char * volatile &tp;
		inline T2(OsRrPrio::BinarySemaphore &sem, char * volatile &tp): sem(sem), tp(tp) {}

		bool run() {
			*tp++ = 'b';
			OsRrPrio::select(&sem);
			*tp++ = 'm';

			return ok;
		}
	} t2(sem, tp);

	struct T3: public TestTask<T3, OsRrPrio> {
		OsRrPrio::Mutex &mutex;
		OsRrPrio::BinarySemaphore &sem;
		char * volatile &tp;
		inline T3(OsRrPrio::Mutex &mutex, OsRrPrio::BinarySemaphore &sem, char * volatile &tp): mutex(mutex), sem(sem), tp(tp) {}

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
	} t3(mutex, sem, tp);

	struct T4: public TestTask<T4, OsRrPrio> {
		OsRrPrio::BinarySemaphore &sem;
		char * volatile &tp;
		inline T4(OsRrPrio::BinarySemaphore &sem, char * volatile &tp): sem(sem), tp(tp) {}

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
	} t4(sem, tp);

	t1.start(uint8_t(0));
	t2.start(uint8_t(1));
	t3.start(uint8_t(2));
	t4.start(uint8_t(3));
	mutex.init();
	sem.init(false);

	CommonTestUtils::start<OsRrPrio>();

	CHECK(pet::Str::ncmp(trace, "abcdefghijklmn", sizeof(trace)) == 0);
}
