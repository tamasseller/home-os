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
    template<class... Args>
    void start(Args... args) {
        Os::Task::template start<Child, &Child::run>(StackPool::get(), testStackSize, args...);
    }
};


#endif /* TESTTASK_H_ */
