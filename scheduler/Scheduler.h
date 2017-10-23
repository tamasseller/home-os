/*
 * Scheduler.h
 *
 *  Created on: 2017.05.24.
 *      Author: tooma
 */

#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <stdint.h>
#include "data/OrderedDoubleList.h"
#include "data/LinkedList.h"
#include "meta/Configuration.h"

#include "policy/RoundRobinPolicy.h"


/*
 * Scheduler root class.
 */

struct SchedulerOptions {
	template<bool Arg>
	struct EnableAssert: pet::ConfigValue<bool, EnableAssert, Arg> {};

	template<class Arg>
	struct HardwareProfile: pet::ConfigType<HardwareProfile, Arg> {};

	template<template<class...> class Arg>
	struct SchedulingPolicy: pet::ConfigTemplate<SchedulingPolicy, Arg> {};

	template<class... Options>
	class Configurable {
		static constexpr bool assertEnabled = EnableAssert<false>::extract<Options...>::value;

		using Profile = typename HardwareProfile<void>::extract<Options...>::type;

		template<class... X>
		using PolicyTemplate = typename SchedulingPolicy<RoundRobinPolicy>::extract<Options...>::template typeTemplate<X...>;


	public:
		using TickType = typename Profile::TickType;

		template<class Data> class Atomic;

		class Task;
		class Mutex;
		class AtomicList;
		class BinarySemaphore;
		class CountingSemaphore;
	private:

		class Blockable;
		using PolicyBase = PolicyTemplate<Task, Blockable>;
		using Priority = typename PolicyBase::Priority;

		class Policy;
		class Sleeper;
		class SleepList;
		class Blocker;

		template<bool (*)(const Blockable&, const Blockable&)> class BlockList;
		class Waitable;
		class WaitableSet;

		class Event;
		class EventList;

		class PreemptionEvent;

		static struct State {
			Policy policy;
			bool isRunning;   // TODO check if can be merged
			uintptr_t nTasks; // TODO check if can be merged
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
		static uintptr_t doSelectTimeout(uintptr_t waitableSet, uintptr_t timeout);

		/*
		 * Internal helpers
		 */
		static inline void assert(bool, const char*);

		template<class T>
		static inline uintptr_t detypePtr(T* x);

		template<class T>
		static inline T* entypePtr(uintptr_t  x);

		template<bool pendOld, bool suspend = true>
		static inline void switchToNext();

		static inline bool firstPreemptsSecond(const Task* first, const Task *second);

	public:
		inline static TickType getTick();

		template<class... T> inline static void start(T... t);

		inline static void yield();
		inline static void sleep(uintptr_t time);
		inline static void exit();
		template<class... T> inline static Waitable* select(T... t);
		template<class... T> inline static Waitable* selectTimeout(uintptr_t timout, T... t);
	};
};

template<class... Args>
using Scheduler = SchedulerOptions::Configurable<Args...>;

#include "internal/Policy.h"
#include "internal/Atomic.h"
#include "internal/AtomicList.h"
#include "internal/Event.h"
#include "internal/EventList.h"
#include "internal/Blocker.h"
#include "internal/Helpers.h"
#include "internal/Sleeper.h"
#include "internal/SleepList.h"
#include "internal/Blockable.h"
#include "internal/Waitable.h"
#include "internal/WaitableSet.h"
#include "internal/Preemption.h"

#include "frontend/Mutex.h"
#include "frontend/Task.h"
#include "frontend/BinarySemaphore.h"
#include "frontend/CountingSemaphore.h"

///////////////////////////////////////////////////////////////////////////////

template<class... Args>
typename Scheduler<Args...>::State Scheduler<Args...>::state;

template<class... Args>
template<class... T>
inline void Scheduler<Args...>::start(T... t) {
	Task* firstTask = state.policy.popNext();
	state.isRunning = true;
	Profile::setTickHandler(&Scheduler<Args...>::onTick);
	Profile::init(t...);
	Profile::startFirst(firstTask);
}

template<class... Args>
inline typename Scheduler<Args...>::Profile::TickType Scheduler<Args...>::getTick() {
	return Profile::getTick();
}

#endif /* SCHEDULER_H_ */
