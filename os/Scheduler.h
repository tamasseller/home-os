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
	inline static void start();
	inline static void yield();
	inline static void exit();

private:
	class TaskBase;
	class MutexBase;

	static TaskBase* currentTask;
	static Policy<TaskBase> policy;
	static bool isRunning;

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

template<class Profile, template<class> class Policy>
Policy<typename Scheduler<Profile, Policy>::TaskBase> Scheduler<Profile, Policy>::policy;

template<class Profile, template<class> class Policy>
typename Scheduler<Profile, Policy>::TaskBase* Scheduler<Profile, Policy>::currentTask;

template<class Profile, template<class> class Policy>
bool Scheduler<Profile, Policy>::isRunning = false;

template<class Profile, template<class> class Policy>
inline void Scheduler<Profile, Policy>::start() {
	currentTask = static_cast<TaskBase*>(policy.getNext());
	isRunning = true;
	currentTask->startFirst();
}

template<class Profile, template<class> class Policy>
inline typename Profile::Timer::TickType Scheduler<Profile, Policy>::getTick() {
	return Profile::Timer::getTick();
}

#endif /* SCHEDULER_H_ */
