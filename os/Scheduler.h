/*
 * Scheduler.h
 *
 *  Created on: 2017.05.24.
 *      Author: tooma
 */

#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include "data/LinkedList.h"

/*
 * Scheduler root class.
 */
template<class Profile, class Policy>
class Scheduler {
public:
	template<class Child>
	class Task;

	class Mutex;

	inline static typename Profile::Timer::TickType getTick();
	inline static void start();
	inline static void yield();
	inline static void exit();

private:
	class TaskBase;
	class MutexBase;

	static TaskBase* currentTask;
	static Policy policy;

	template<class T>
	static uintptr_t detypePtr(T* x);

	template<class T>
	static T* entypePtr(uintptr_t  x);

	inline static bool switchToNext();
	static uintptr_t doStartTask(uintptr_t task);
	static uintptr_t doExit();
	static uintptr_t doYield();
	static uintptr_t doLock(uintptr_t mutex);
	static uintptr_t doUnlock(uintptr_t mutex);
};

/*
 * Public nested classes.
 */

template<class Profile, class Policy>
template<class Child>
class Scheduler<Profile, Policy>::Task: public TaskBase {
public:
	inline void start(void* stack, uint32_t stackSize);
};

template<class Profile, class Policy>
class Scheduler<Profile, Policy>::Mutex: public MutexBase {
public:
	void init();
	void lock();
	void unlock();
};


/*
 * Public methods.
 */

template<class Profile, class Policy>
inline void Scheduler<Profile, Policy>::start() {
	currentTask = static_cast<TaskBase*>(policy.getNext());
	currentTask->startFirst();
}

template<class Profile, class Policy>
inline void Scheduler<Profile, Policy>::yield() {
	Profile::CallGate::sync(&Scheduler::doYield);
}

template<class Profile, class Policy>
inline void Scheduler<Profile, Policy>::exit() {
	Profile::CallGate::sync(&Scheduler::doExit);
}

template<class Profile, class Policy>
inline typename Profile::Timer::TickType Scheduler<Profile, Policy>::getTick() {
	return Profile::Timer::getTick();
}

template<class Profile, class Policy>
template<class Child>
inline void Scheduler<Profile, Policy>::Task<Child>::start(void* stack, uint32_t stackSize) {
	Profile::Task::template initialize<
		Child,
		&Child::run,
		&Scheduler::exit> (
				stack,
				stackSize,
				static_cast<Child*>(this));

	Profile::CallGate::sync(
			&Scheduler<Profile, Policy>::doStartTask,
			detypePtr(static_cast<TaskBase*>(this)));
}

template<class Profile, class Policy>
void Scheduler<Profile, Policy>::Mutex::init() {
	MutexBase::init();
}

template<class Profile, class Policy>
void Scheduler<Profile, Policy>::Mutex::lock() {
	Profile::CallGate::sync(&Scheduler::doLock, detypePtr(static_cast<MutexBase*>(this)));
}

template<class Profile, class Policy>
void Scheduler<Profile, Policy>::Mutex::unlock() {
	Profile::CallGate::sync(&Scheduler::doUnlock, detypePtr(static_cast<MutexBase*>(this)));
}

/*
 * Internal nested classes.
 */

template<class Profile, class Policy>
class Scheduler<Profile, Policy>::TaskBase: Profile::Task, Policy::Task {
	friend Scheduler;
	friend pet::LinkedList<TaskBase>;

	TaskBase* next;
};

template<class Profile, class Policy>
class Scheduler<Profile, Policy>::MutexBase {
	friend Scheduler;
	TaskBase *owner;
	pet::LinkedList<TaskBase> waiters;
	uint32_t counter;

	void init();
};

/*
 * Internal workers.
 */

template<class Profile, class Policy>
inline bool Scheduler<Profile, Policy>::switchToNext()
{
	if(TaskBase* newTask = static_cast<TaskBase*>(policy.getNext())) {
		if(newTask != currentTask) {
			currentTask->switchTo(newTask);
			currentTask = newTask;
			return true;
		}
	}

	return false;
}

template<class Profile, class Policy>
uintptr_t Scheduler<Profile, Policy>::doStartTask(uintptr_t task) {
	policy.addRunnable(entypePtr<TaskBase>(task));
}

template<class Profile, class Policy>
uintptr_t Scheduler<Profile, Policy>::doExit() {
	if(!switchToNext())
		currentTask->finishLast();
}

template<class Profile, class Policy>
uintptr_t Scheduler<Profile, Policy>::doYield() {
	policy.addRunnable(currentTask);
	switchToNext();
}

template<class Profile, class Policy>
void Scheduler<Profile, Policy>::MutexBase::init()
{
	owner = nullptr;
}

template<class Profile, class Policy>
uintptr_t Scheduler<Profile, Policy>::doLock(uintptr_t mutexPtr)
{
	MutexBase* mutex = entypePtr<MutexBase>(mutexPtr);

	if(!mutex->owner) {
		mutex->owner = currentTask;
		mutex->counter = 0;
		// assert(mutex->waiters.empty();
	} else if(mutex->owner == currentTask) {
		mutex->counter++;
	} else {
		mutex->waiters.add(currentTask);
		bool ok = switchToNext();
		// assert(ok);
	}
}

template<class Profile, class Policy>
uintptr_t Scheduler<Profile, Policy>::doUnlock(uintptr_t mutexPtr)
{
	MutexBase* mutex = entypePtr<MutexBase>(mutexPtr);

	// assert(mutex->owner == currentTask);

	if(mutex->counter)
		mutex->counter--;
	else {
		auto it = mutex->waiters.iterator();
		while(it.current() && it.current()->next)
			it.step();

		if(TaskBase* waken = it.current()) {
			it.remove();
			mutex->owner = waken;
			policy.addRunnable(waken);
			if(!policy.canRunMore(currentTask)) {
				policy.addRunnable(currentTask);
				switchToNext();
			}
		} else {
			mutex->owner = nullptr;
		}
	}
}

/*
 * Helpers.
 */

template<class Profile, class Policy>
template<class T>
uintptr_t Scheduler<Profile, Policy>::detypePtr(T* x) {
	return reinterpret_cast<uintptr_t>(x);
}

template<class Profile, class Policy>
template<class T>
T* Scheduler<Profile, Policy>::entypePtr(uintptr_t  x) {
	return reinterpret_cast<T*>(x);
}


template<class Profile, class Policy>
Policy Scheduler<Profile, Policy>::policy;

template<class Profile, class Policy>
typename Scheduler<Profile, Policy>::TaskBase* Scheduler<Profile, Policy>::currentTask;


#endif /* SCHEDULER_H_ */
