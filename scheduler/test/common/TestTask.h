/*
 * TestTask.h
 *
 *      Author: tamas.seller
 */

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
