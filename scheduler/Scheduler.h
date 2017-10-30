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

	template<bool Arg>
	struct EnableRegistry: pet::ConfigValue<bool, EnableRegistry, Arg> {};

	template<class Arg>
	struct HardwareProfile: pet::ConfigType<HardwareProfile, Arg> {};

	template<template<class...> class Arg>
	struct SchedulingPolicy: pet::ConfigTemplate<SchedulingPolicy, Arg> {};

	template<class... Options>
	class Configurable {
		static constexpr bool assertEnabled = EnableAssert<false>::extract<Options...>::value;
		static constexpr bool registryEnabled = EnableRegistry<assertEnabled>::template extract<Options...>::value;

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
		class Blocker;
		class SharedBlocker;
		template<class> class AsyncBlocker;
		template<class> class SemaphoreLikeBlocker;
		class WaitableSet;

		class Policy;
		class Sleeper;
		class SleepList;

		class Event;
		class EventList;
		class PreemptionEvent;

		template<class, bool> class ObjectRegistry;

		template<class Object> using Registry = ObjectRegistry<Object, registryEnabled>;
		using PolicyBase = PolicyTemplate<Task, Blockable>;

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
		template<class> static uintptr_t doRegisterObject(uintptr_t object);
		template<class> static uintptr_t doUnregisterObject(uintptr_t object);
		template<class> static uintptr_t doBlock(uintptr_t blocker);
		template<class> static uintptr_t doTimedBlock(uintptr_t blocker, uintptr_t time);
		template<class> static uintptr_t doRelease(uintptr_t blocker, uintptr_t arg);

		template<class M, class... T> static uintptr_t syscall(M, T...);
		template<class M, class... T> static inline uintptr_t conditionalSyscall(M, T... );

		/*
		 * Internal helpers
		 */
		static inline void assert(bool, const char*);

		template<bool pendOld, bool suspend = true> static inline void switchToNext();

		template<class...> class SyscallMap;
		static inline void* syscallMapper(uintptr_t);

		static inline Task* getCurrentTask();

	public:
		inline static TickType getTick();

		template<class... T> inline static void start(T... t);

		inline static void yield();
		inline static void sleep(uintptr_t time);
		inline static void exit();
		template<class... T> inline static Blocker* select(T... t);
		template<class... T> inline static Blocker* selectTimeout(uintptr_t timout, T... t);
	};
};

template<class... Args>
using Scheduler = SchedulerOptions::Configurable<Args...>;

#include "internal/Atomic.h"
#include "internal/AtomicList.h"
#include "internal/Event.h"
#include "internal/EventList.h"
#include "internal/Preemption.h"

#include "blocking/Policy.h"
#include "blocking/Blocker.h"
#include "blocking/AsyncBlocker.h"
#include "blocking/SharedBlocker.h"
#include "blocking/SemaphoreLikeBlocker.h"
#include "blocking/Sleeper.h"
#include "blocking/SleepList.h"
#include "blocking/Blockable.h"
#include "blocking/WaitableSet.h"

#include "syscall/Helpers.h"
#include "syscall/Syscall.h"
#include "syscall/ObjectRegistry.h"

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
	Profile::setSyscallMapper(&syscallMapper);
	Profile::startFirst(firstTask);
}

template<class... Args>
inline typename Scheduler<Args...>::Profile::TickType Scheduler<Args...>::getTick() {
	return Profile::getTick();
}

#endif /* SCHEDULER_H_ */
