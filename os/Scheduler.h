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

		template<class Data> class Atomic;

		class Task;
		class Mutex;
		class AtomicList;
		class BinarySemaphore;
		class CountingSemaphore;
	private:

		class Blockable;
		class Sleeper;
		class SleepList;
		class Waker;
		class Waitable;
		class WaitableSet;

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
		static uintptr_t doStartTask(uintptr_t task);
		static uintptr_t doExit();
		static uintptr_t doYield();
		static uintptr_t doSleep(uintptr_t time);
		static uintptr_t doLock(uintptr_t mutex);
		static uintptr_t doUnlock(uintptr_t mutex);
		static uintptr_t doWait(uintptr_t waitable);
		static uintptr_t doWaitTimeout(uintptr_t waitable, uintptr_t timeout);
		static uintptr_t doSelect(uintptr_t waitableSet);

		template<class T>
		static inline uintptr_t detypePtr(T* x);

		template<class T>
		static inline T* entypePtr(uintptr_t  x);

		template<bool pendOld, bool suspend = true>
		static inline void switchToNext();

		static inline bool firstPreemptsSecond(Task* first, Task *second);

	public:
		inline static TickType getTick();

		template<class... T> inline static void start(T... t);


		inline static void yield();
		inline static void sleep(uintptr_t time);
		inline static void exit();
		template<class... T> inline static Waitable* select(T... t);
	};
};

template<class... Args>
using Scheduler = SchedulerOptions::Configurable<Args...>;

#include "internal/Atomic.h"
#include "internal/AtomicList.h"
#include "internal/Events.h"
#include "internal/Waker.h"
#include "internal/Helpers.h"
#include "internal/Sleepers.h"
#include "internal/Blockable.h"
#include "internal/Waitable.h"
#include "internal/WaitableSet.h"

#include "Mutex.h"
#include "Scheduler.h"
#include "Task.h"
#include "Semaphore.h"

///////////////////////////////////////////////////////////////////////////////

template<class... Args>
class Scheduler<Args...>::PreemptionEvent: public Scheduler<Args...>::Event {
	static inline void execute(Event* self, uintptr_t arg) {
		// assert(arg == 1);

		while(Sleeper* sleeper = state.sleepList.getWakeable()) {
			Task* task = static_cast<Task*>(sleeper);
			if(task->waitsFor) {
				task->waitsFor->remove(task);
				task->waitsFor = nullptr;
			}
			state.policy.addRunnable(task);
		}

		if(Task* newTask = state.policy.peekNext()) {
			if(typename Profile::Task* platformTask = Profile::Task::getCurrent()) {

				Task* currentTask = static_cast<Task*>(platformTask);

				if(firstPreemptsSecond(currentTask, newTask))
					return;

				state.policy.addRunnable(static_cast<Task*>(currentTask));
			}

			state.policy.popNext();
			static_cast<Task*>(newTask)->switchToAsync();
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
	state.eventList.issue(&state.preemptionEvent);
}

#endif /* SCHEDULER_H_ */
