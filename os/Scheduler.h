/*
 * Scheduler.h
 *
 *  Created on: 2017.05.24.
 *      Author: tooma
 */

#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <stdint.h>

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
	inline static void sleep(uintptr_t time);
	inline static void exit();

private:
	class Sleeper;
	class TaskBase;
	class SleepList;
	class EventBase;
	class EventList;
	class MutexBase;
	class AtomicList;
	class PreemptionEvent;
	template<class> class Event;

	static bool isRunning;
	static uintptr_t nTasks;
	static EventList eventList;
	static SleepList sleepList;
	static Policy<TaskBase> policy;
	static PreemptionEvent preemptionEvent;

	static void onTick();
	static void doAsync();
	static uintptr_t doStartTask(uintptr_t task);
	static uintptr_t doExit();
	static uintptr_t doYield();
	static uintptr_t doSleep(uintptr_t time);
	static uintptr_t doLock(uintptr_t mutex);
	static uintptr_t doUnlock(uintptr_t mutex);

	template<class T>
	static inline uintptr_t detypePtr(T* x);

	template<class T>
	static inline T* entypePtr(uintptr_t  x);

	template<bool pendOld>
	static inline void switchToNext();

	template<class RealEvent, class... Args>
	static inline void postEvent(RealEvent*, Args... args);
};

#include "AtomicList.h"
#include "Helpers.h"
#include "Mutex.h"
#include "Task.h"
#include "Event.h"
#include "SleepList.h"

///////////////////////////////////////////////////////////////////////////////

template<class Profile, template<class> class Policy>
class Scheduler<Profile, Policy>::PreemptionEvent:
public Scheduler<Profile, Policy>::template Event<Scheduler<Profile, Policy>::PreemptionEvent> {
public:
	class Combiner {
	public:
		inline bool operator()(uintptr_t old, uintptr_t& result) const {
			result = old+1;
			return true;
		}
	};

	static inline void execute(uintptr_t arg) {
		// assert(arg == 1);

		while(TaskBase* sleeper = sleepList.peek()) {
			if(!(*sleeper < Profile::Timer::getTick()))
				break;

			sleepList.pop();
			policy.addRunnable(static_cast<TaskBase*>(sleeper));
		}

		if(typename Profile::Task* platformTask = Profile::Task::getCurrent()) {
			if(TaskBase* newTask = policy.peekNext()) {
				TaskBase* currentTask = static_cast<TaskBase*>(platformTask);
				if(!policy.isHigherPriority(currentTask, newTask)) {
					policy.popNext();
					policy.addRunnable(static_cast<TaskBase*>(currentTask));
					newTask->switchTo();
				}
			}
		} else {
			if(TaskBase* newTask = policy.popNext())
				newTask->switchTo();
		}
	}
};

template<class Profile, template<class> class Policy>
class Scheduler<Profile, Policy>::PreemptionEvent Scheduler<Profile, Policy>::preemptionEvent;

template<class Profile, template<class> class Policy>
typename Scheduler<Profile, Policy>::SleepList Scheduler<Profile, Policy>::sleepList;

template<class Profile, template<class> class Policy>
typename Scheduler<Profile, Policy>::EventList Scheduler<Profile, Policy>::eventList;

template<class Profile, template<class> class Policy>
Policy<typename Scheduler<Profile, Policy>::TaskBase> Scheduler<Profile, Policy>::policy;

template<class Profile, template<class> class Policy>
bool Scheduler<Profile, Policy>::isRunning = false;

template<class Profile, template<class> class Policy>
uintptr_t Scheduler<Profile, Policy>::nTasks;


template<class Profile, template<class> class Policy>
template<class... T>
inline void Scheduler<Profile, Policy>::start(T... t) {
	TaskBase* firstTask = policy.popNext();
	isRunning = true;
	Profile::Timer::setTickHandler(&Scheduler::onTick);
	Profile::init(t...);
	firstTask->startFirst();
}

template<class Profile, template<class> class Policy>
inline typename Profile::Timer::TickType Scheduler<Profile, Policy>::getTick() {
	return Profile::Timer::getTick();
}

template<class Profile, template<class> class Policy>
void Scheduler<Profile, Policy>::onTick()
{
	postEvent(&preemptionEvent);
}

template<class Profile, template<class> class Policy>
inline void Scheduler<Profile, Policy>:: doAsync()
{
	eventList.dispatch();
}

#endif /* SCHEDULER_H_ */
