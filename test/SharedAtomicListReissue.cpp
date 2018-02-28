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

TEST(AtomicListReissue) {
	OsRr::SharedAtomicList list;

	struct Element: OsRr::SharedAtomicList::Element {
		home::Atomic<int> data = 0;
		uintptr_t argMax = 0;
		void work(uintptr_t arg) {
			if(argMax < arg)
				argMax = arg;

			AtomicUtils::increment(data, static_cast<int>(arg));
		}
	};

	Element e1, e2, e3;

	struct Task: public TestTask<Task> {
		OsRr::SharedAtomicList &list;
		Element &e1, &e2, &e3;
		inline Task(OsRr::SharedAtomicList list, Element &e1, Element &e2, Element &e3): list(list), e1(e1), e2(e2), e3(e3) {}

		bool run() {
			auto combiner = [](uintptr_t old, uintptr_t &result){
				result = old+1;
				return true;
			};

			for(int i = 0; i < 4096/3; i++) {
				list.push(&e1, combiner);
				list.push(&e2, combiner);
				list.push(&e3, combiner);

				OsRr::yield();

				uintptr_t arg;
				auto it = list.read();
				while(Element* x = (Element*)it.pop(arg))
					x->work(arg);
			}

			return ok;
		}
	} t1(list, e1, e2, e3), t2(list, e1, e2, e3), t3(list, e1, e2, e3);

	t1.start();
	t2.start();
	t3.start();

	CommonTestUtils::start();

	CHECK(e1.argMax > 1 || e2.argMax > 1 || e3.argMax > 1);
	CHECK(e1.data == 4096/3*3);
	CHECK(e2.data == 4096/3*3);
	CHECK(e3.data == 4096/3*3);
}
