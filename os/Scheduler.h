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

#include "Helpers.h"
#include "Mutex.h"
#include "Task.h"

///////////////////////////////////////////////////////////////////////////////

template<class Profile, class Policy>
inline void Scheduler<Profile, Policy>::start() {
	currentTask = static_cast<TaskBase*>(policy.getNext());
	currentTask->startFirst();
}

template<class Profile, class Policy>
inline typename Profile::Timer::TickType Scheduler<Profile, Policy>::getTick() {
	return Profile::Timer::getTick();
}

/*
 * Internal nested classes.
 */

/*
 * Internal workers.
 */

template<class Profile, class Policy>
Policy Scheduler<Profile, Policy>::policy;

template<class Profile, class Policy>
typename Scheduler<Profile, Policy>::TaskBase* Scheduler<Profile, Policy>::currentTask;


#endif /* SCHEDULER_H_ */
