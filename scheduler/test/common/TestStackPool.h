/*
 * TestStackPool.h
 *
 *      Author: tamas.seller
 */

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
