/*
 * AtomicList.cpp
 *
 *  Created on: 2017.06.04.
 *      Author: tooma
 */

#include "CommonTestUtils.h"

namespace {
	OsRr::AtomicList list;

	bool error = false;
	bool otherOnesDone = false;
	uintptr_t argMax = 0;

	struct Element: OsRr::AtomicList::Element {
		uintptr_t data = 0;
		void work(uintptr_t arg) {
			if(argMax < arg)
				argMax = arg;

			data += arg;
		}
	};

	struct Task: public TestTask<Task> {
		Element e1, e2, e3;

		void run() {
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
						error = true;

					n++;
				}

				if(n > 3)
					otherOnesDone = true;
			}
		}
	} t1, t2;
}

TEST(AtomicList) {
	t1.start();
	t2.start();

	CommonTestUtils::start();

	CHECK(!error);
	CHECK(otherOnesDone);
	CHECK(argMax == 1);
	CHECK(t1.e1.data == UINT16_MAX-1);
	CHECK(t1.e2.data == UINT16_MAX-1);
	CHECK(t1.e3.data == UINT16_MAX-1);
	CHECK(t2.e1.data == UINT16_MAX-1);
	CHECK(t2.e2.data == UINT16_MAX-1);
	CHECK(t2.e3.data == UINT16_MAX-1);
}
