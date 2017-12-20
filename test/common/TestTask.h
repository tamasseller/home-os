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

#ifndef TESTTASK_H_
#define TESTTASK_H_

typedef TestStackPool<testStackSize, testStackCount> StackPool;

template<class Child, class Os=OsRr>
struct TestTask: Os::Task
{
	static constexpr bool bad = true;
	static constexpr bool ok = false;
	bool error;

	static inline void runTest(void* arg) {
		Child* self = reinterpret_cast<Child*>(arg);
		self->error = self->run();
	}

	template<class C>
	static void doIndirect(C c) {
		struct Event: Os::Event {
			C c;
			static void callback(typename Os::Event* x, uintptr_t) {
				static_cast<Event*>(x)->c();
			}

			Event(C c): Os::Event(&Event::callback), c(c) {}
		} event(c);

		Os::submitEvent(&event);
	}

    template<class... Args>
    void start(Args... args) {
        Os::Task::start(&TestTask::runTest, StackPool::get(), testStackSize, static_cast<Child*>(this), args...);
    }
};


#endif /* TESTTASK_H_ */
