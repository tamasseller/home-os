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
template<class Profile, template<class> class PolicyParam>
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
	class SleepList;

	class Waiter;
	class WaitList;

	class EventBase;
	class EventList;
	class AtomicList;
	class PreemptionEvent;
	template<class> class Event;

	class TaskBase;
	class MutexBase;

	typedef PolicyParam<Waiter> Policy;

	static Policy policy;
	static bool isRunning;
	static uintptr_t nTasks;
	static EventList eventList;
	static SleepList sleepList;
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

#include "WaitList.h"
#include "SleepList.h"
#include "AtomicList.h"
#include "Helpers.h"
#include "Mutex.h"
#include "Task.h"
#include "Event.h"


///////////////////////////////////////////////////////////////////////////////

template<class Profile, template<class> class PolicyParam>
class Scheduler<Profile, PolicyParam>::PreemptionEvent:
public Scheduler<Profile, PolicyParam>::template Event<Scheduler<Profile, PolicyParam>::PreemptionEvent> {
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

		while(Sleeper* sleeper = sleepList.peek()) {
			if(!(*sleeper < Profile::Timer::getTick()))
				break;

			sleepList.pop();
			TaskBase* task = static_cast<TaskBase*>(sleeper);
			if(task->isWaiting()) {
				// assert(task->waitList);
				task->waitList->remove(task);
			}
			policy.addRunnable(task);
		}

		if(typename Profile::Task* platformTask = Profile::Task::getCurrent()) {
			if(Waiter* newTask = policy.peekNext()) {
				TaskBase* currentTask = static_cast<TaskBase*>(platformTask);
				if(*static_cast<Waiter*>(currentTask) < *newTask) {
					policy.popNext();
					policy.addRunnable(static_cast<TaskBase*>(currentTask));
					static_cast<TaskBase*>(newTask)->switchTo();
				}
			}
		} else {
			if(TaskBase* newTask = static_cast<TaskBase*>(policy.popNext()))
				newTask->switchTo();
		}
	}
};

template<class Profile, template<class> class PolicyParam>
class Scheduler<Profile, PolicyParam>::PreemptionEvent Scheduler<Profile, PolicyParam>::preemptionEvent;

template<class Profile, template<class> class PolicyParam>
typename Scheduler<Profile, PolicyParam>::SleepList Scheduler<Profile, PolicyParam>::sleepList;

template<class Profile, template<class> class PolicyParam>
typename Scheduler<Profile, PolicyParam>::EventList Scheduler<Profile, PolicyParam>::eventList;

template<class Profile, template<class> class PolicyParam>
typename Scheduler<Profile, PolicyParam>::Policy Scheduler<Profile, PolicyParam>::policy;

template<class Profile, template<class> class PolicyParam>
bool Scheduler<Profile, PolicyParam>::isRunning = false;

template<class Profile, template<class> class PolicyParam>
uintptr_t Scheduler<Profile, PolicyParam>::nTasks;


template<class Profile, template<class> class PolicyParam>
template<class... T>
inline void Scheduler<Profile, PolicyParam>::start(T... t) {
	TaskBase* firstTask = static_cast<TaskBase*>(static_cast<TaskBase*>(policy.popNext()));
	isRunning = true;
	Profile::Timer::setTickHandler(&Scheduler::onTick);
	Profile::init(t...);
	firstTask->startFirst();
}

template<class Profile, template<class> class PolicyParam>
inline typename Profile::Timer::TickType Scheduler<Profile, PolicyParam>::getTick() {
	return Profile::Timer::getTick();
}

template<class Profile, template<class> class PolicyParam>
void Scheduler<Profile, PolicyParam>::onTick()
{
	postEvent(&preemptionEvent);
}

template<class Profile, template<class> class PolicyParam>
inline void Scheduler<Profile, PolicyParam>:: doAsync()
{
	eventList.dispatch();
}

#endif /* SCHEDULER_H_ */
