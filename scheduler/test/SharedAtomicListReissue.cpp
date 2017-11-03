/*
 * AtomicListReissue.cpp
 *
 *  Created on: 2017.06.04.
 *      Author: tooma
 */

#include "common/CommonTestUtils.h"

namespace {
	OsRr::SharedAtomicList list;

	uintptr_t argMax = 0;

	struct Element: OsRr::SharedAtomicList::Element {
		OsRr::Atomic<int> data = 0;
		void work(uintptr_t arg) {
			if(argMax < arg)
				argMax = arg;

			data.increment(static_cast<int>(arg));
		}
	};

	Element e1, e2, e3;

	struct Task: public TestTask<Task> {

		void run() {
			auto combiner = [](uintptr_t old, uintptr_t &result){
				result = old+1;
				return true;
			};

			for(int i = 0; i < UINT16_MAX/3; i++) {
				list.push(&e1, combiner);
				list.push(&e2, combiner);
				list.push(&e3, combiner);

				OsRr::yield();

				uintptr_t arg;
				auto it = list.read();
				while(Element* x = (Element*)it.pop(arg))
					x->work(arg);
			}
		}
	} t1, t2, t3;
}

TEST(AtomicListReissue) {
	t1.start();
	t2.start();
	t3.start();

	CommonTestUtils::start();

	CHECK(argMax > 1);
	CHECK(e1.data == UINT16_MAX/3*3);
	CHECK(e2.data == UINT16_MAX/3*3);
	CHECK(e3.data == UINT16_MAX/3*3);
}
