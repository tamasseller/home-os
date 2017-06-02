/*
 * Scheduler.h
 *
 *  Created on: 2017.05.24.
 *      Author: tooma
 */

#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <stdint.h>

#include "meta/Configuration.h"

#include "policy/RoundRobinPolicy.h"

/*
 * Scheduler root class.
 */

struct SchedulerOptions {
	template<class Arg>
	struct HardwareProfile: pet::ConfigType<HardwareProfile, Arg> {};

	template<template<class...> class Arg>
	struct SchedulingPolicy: pet::ConfigTemplate<SchedulingPolicy, Arg> {};

	template<class... Options>
	class Configurable {
		using Profile = typename HardwareProfile<void>::extract<Options...>::type;

		template<class... X>
		using PolicyTemplate = typename SchedulingPolicy<RoundRobinPolicy>::extract<Options...>::template typeTemplate<X...>;

	public:
		using TickType = typename Profile::Timer::TickType;

		class Task;
		class Mutex;
		class Waitable;
		class AtomicList;

		inline static TickType getTick();

		template<class... T> inline static void start(T... t);

		inline static void yield();
		inline static void sleep(uintptr_t time);
		inline static void exit();

	private:
		class Blockable;
		class Sleeper;
		class SleepList;

		class Event;
		class EventList;

		class PreemptionEvent;

		typedef PolicyTemplate<Task, Blockable> Policy;

		static struct State {
			Policy policy;
			bool isRunning;
			uintptr_t nTasks;
			EventList eventList;
			SleepList sleepList;
			PreemptionEvent preemptionEvent;
		} state;

		static void onTick();
		static void doAsync();
		static uintptr_t doStartTask(uintptr_t task);
		static uintptr_t doExit();
		static uintptr_t doYield();
		static uintptr_t doSleep(uintptr_t time);
		static uintptr_t doLock(uintptr_t mutex);
		static uintptr_t doUnlock(uintptr_t mutex);
		static uintptr_t doBlock(uintptr_t waitable, uintptr_t timeout);
		static uintptr_t doNotify(uintptr_t timeout);

		template<class T>
		static inline uintptr_t detypePtr(T* x);

		template<class T>
		static inline T* entypePtr(uintptr_t  x);

		template<bool pendOld>
		static inline void switchToNext();

		template<class RealEvent, class... Args>
		static inline void postEvent(RealEvent*, Args... args);

	};
};

template<class... Args>
using Scheduler = SchedulerOptions::Configurable<Args...>;

#include "internal/AtomicList.h"
#include "internal/Events.h"
#include "internal/Helpers.h"
#include "internal/Sleepers.h"
#include "internal/Blocking.h"

#include "Mutex.h"
#include "Scheduler.h"
#include "Task.h"
#include "Waitable.h"
//#include "Blockable.h"

///////////////////////////////////////////////////////////////////////////////

template<class... Args>
class Scheduler<Args...>::PreemptionEvent: public Scheduler<Args...>::Event {
	static inline void execute(uintptr_t arg) {
		// assert(arg == 1);

		while(Sleeper* sleeper = state.sleepList.getWakeable()) {
			Task* task = static_cast<Task*>(sleeper);
			if(task->waitsFor) {
				// TODO
				// task->waitsFor->interrupted();
			}
			state.policy.addRunnable(task);
		}

		if(typename Profile::Task* platformTask = Profile::Task::getCurrent()) {
			if(Task* newTask = state.policy.peekNext()) {
				Task* currentTask = static_cast<Task*>(platformTask);
				if(*currentTask < *newTask) {
					state.policy.popNext();
					state.policy.addRunnable(static_cast<Task*>(currentTask));
					static_cast<Task*>(newTask)->switchTo();
				}
			}
		} else {
			if(Task* newTask = static_cast<Task*>(state.policy.popNext()))
				newTask->switchTo();
		}
	}

public:
	inline PreemptionEvent(): Event(execute) {}
};

template<class... Args>
typename Scheduler<Args...>::State Scheduler<Args...>::state;

template<class... Args>
template<class... T>
inline void Scheduler<Args...>::start(T... t) {
	Task* firstTask = state.policy.popNext();
	state.isRunning = true;
	Profile::Timer::setTickHandler(&Scheduler<Args...>::onTick);
	Profile::init(t...);
	firstTask->startFirst();
}

template<class... Args>
inline typename Scheduler<Args...>::Profile::Timer::TickType Scheduler<Args...>::getTick() {
	return Profile::Timer::getTick();
}

template<class... Args>
void Scheduler<Args...>::onTick()
{
	postEvent(&state.preemptionEvent);
}

template<class... Args>
inline void Scheduler<Args...>:: doAsync()
{
	state.eventList.dispatch();
}

#endif /* SCHEDULER_H_ */
