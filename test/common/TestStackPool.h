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

#ifndef TESTSTACKPOOL_H_
#define TESTSTACKPOOL_H_

template<uintptr_t size, uintptr_t count>
struct TestStackPool {
    static uintptr_t stacks[size * count / sizeof(uintptr_t)];
    static uintptr_t *current;

    static void* get() {
        void *ret = current;
        current += size / sizeof(uintptr_t);
        return ret;
    }

    static void clear() {
        for(unsigned int i=0; i<sizeof(stacks)/sizeof(stacks[0]); i++)
            stacks[i] = 0x1badc0de;
        current = stacks;
    }
};

template<uintptr_t size, uintptr_t count>
uintptr_t TestStackPool<size, count>::stacks[];

template<uintptr_t size, uintptr_t count>
uintptr_t *TestStackPool<size, count>::current = TestStackPool<size, count>::stacks;


#endif /* TESTSTACKPOOL_H_ */
