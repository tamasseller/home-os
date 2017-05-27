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
template<class Profile, template<class> class Policy>
class Scheduler {
public:
	template<class Child>
	class Task;

	class Mutex;

	inline static typename Profile::Timer::TickType getTick();

	template<class... T> inline static void start(T... t);
	inline static void yield();
	inline static void exit();

private:
	class TaskBase;
	class MutexBase;

	static Policy<TaskBase> policy;
	static bool isRunning;

	template<class T>
	static uintptr_t detypePtr(T* x);

	template<class T>
	static T* entypePtr(uintptr_t  x);

	template<bool pendOld>
	inline static void switchToNext();

	static void doAsync();
	static void doPreempt();
	static uintptr_t doStartTask(uintptr_t task);
	static uintptr_t doExit();
	static uintptr_t doYield();
	static uintptr_t doLock(uintptr_t mutex);
	static uintptr_t doUnlock(uintptr_t mutex);
};

#include "Helpers.h"
#include "Mutex.h"
#include "Task.h"

///////////////////////////////////////////////////////////////////////////////

template<class Profile, template<class> class Policy>
Policy<typename Scheduler<Profile, Policy>::TaskBase> Scheduler<Profile, Policy>::policy;

template<class Profile, template<class> class Policy>
bool Scheduler<Profile, Policy>::isRunning = false;

template<class Profile, template<class> class Policy>
template<class... T>
inline void Scheduler<Profile, Policy>::start(T... t) {
	TaskBase* firstTask = policy.getNext();
	isRunning = true;
	Profile::CallGate::async(&Scheduler::doAsync);
	Profile::Timer::setTickHandler(&Scheduler::doPreempt);
	Profile::init(t...);
	firstTask->startFirst();
}

template<class Profile, template<class> class Policy>
inline typename Profile::Timer::TickType Scheduler<Profile, Policy>::getTick() {
	return Profile::Timer::getTick();
}

template<class Profile, template<class> class Policy>
void Scheduler<Profile, Policy>::doPreempt()
{
	TaskBase* currentTask = static_cast<TaskBase*>(Profile::Task::getCurrent());

	if(TaskBase* newTask = policy.getNext()) {
		if(!policy.isHigherPriority(currentTask, newTask)) {
			policy.addRunnable(static_cast<TaskBase*>(currentTask));
			newTask->switchTo();
		}
	}
}

template<class Profile, template<class> class Policy>
inline void Scheduler<Profile, Policy>:: doAsync()
{
	asm ("nop");
}

#endif /* SCHEDULER_H_ */
