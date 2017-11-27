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
	OsRr::SharedAtomicList list;

	bool otherOnesDone = false;
	uintptr_t argMax = 0;

	struct Element: OsRr::SharedAtomicList::Element {
		uintptr_t data = 0;
		void work(uintptr_t arg) {
			if(argMax < arg)
				argMax = arg;

			data += arg;
		}
	};

	struct Task: public TestTask<Task> {
		Element e1, e2, e3;

		bool run() {
			auto combiner = [](uintptr_t old, uintptr_t &result){
				result = old+1;
				return true;
			};

			for(int i = 0; i < UINT16_MAX-1; i++) {
				list.push(&e1, combiner);
				list.push(&e2, combiner);
				list.push(&e3, combiner);

				uintptr_t arg;
				auto it = list.read();
				int n = 0;
				while(Element* x = (Element*)it.pop(arg)) {
					x->work(arg);

					if(!(e1.data >= e2.data && e2.data >= e3.data))
						return bad;

					n++;
				}

				if(n > 3)
					otherOnesDone = true;
			}

			return ok;
		}
	} t1, t2;
}

TEST(AtomicList) {
	t1.start();
	t2.start();

	CommonTestUtils::start();

	CHECK(otherOnesDone);
	CHECK(argMax == 1);
	CHECK(!t1.error);
	CHECK(t1.e1.data == UINT16_MAX-1);
	CHECK(t1.e2.data == UINT16_MAX-1);
	CHECK(t1.e3.data == UINT16_MAX-1);
	CHECK(!t2.error);
	CHECK(t2.e1.data == UINT16_MAX-1);
	CHECK(t2.e2.data == UINT16_MAX-1);
	CHECK(t2.e3.data == UINT16_MAX-1);
}
