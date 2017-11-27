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

using Os=OsRr;

TEST(MutexDeadlock) {
	Os::Mutex m1, m2;

	struct Task1: public TestTask<Task1> {
		Os::Mutex &m1, &m2;
		inline Task1(Os::Mutex &m1, Os::Mutex &m2): m1(m1), m2(m2) {}

		bool run() {
			m1.lock();
			Os::yield();
			m2.lock();
			while(true);

			return ok;
		}
	} t1(m1, m2);

	struct Task2: public TestTask<Task2> {
		Os::Mutex &m1, &m2;
		inline Task2(Os::Mutex &m1, Os::Mutex &m2): m1(m1), m2(m2) {}

		bool run() {
			m2.lock();
			Os::yield();
			m1.lock();
			while(true);

			return ok;
		}
	} t2(m1, m2);

	m1.init();
	m2.init();
	t1.start();
	t2.start();
	CommonTestUtils::start(Os::ErrorStrings::mutexDeadlock);
}
