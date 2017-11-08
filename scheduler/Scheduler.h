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
#include "data/BinaryHeap.h"

#include "meta/Configuration.h"

#include "policy/RoundRobinPolicy.h"


/*
 * Scheduler root class.
 */

struct SchedulerOptions {

	enum class ScalabilityHint {
		Many, Few
	};

	PET_CONFIG_VALUE(EnableAssert, bool);
	PET_CONFIG_VALUE(EnableRegistry, bool);
	PET_CONFIG_VALUE(EnableDeadlockDetection, bool);
	PET_CONFIG_VALUE(NumberOfSleepers, ScalabilityHint);
	PET_CONFIG_TYPE(HardwareProfile);
	PET_CONFIG_TEMPLATE(SchedulingPolicy);

	template<class... Options>
	class Configurable {
		PET_EXTRACT_VALUE(assertEnabled, EnableAssert, false, Options);
		PET_EXTRACT_VALUE(registryEnabled, EnableAssert, assertEnabled, Options);
		PET_EXTRACT_VALUE(deadlockDetectionEnabled, EnableDeadlockDetection, assertEnabled, Options);
		PET_EXTRACT_VALUE(sleeperStorageOption, NumberOfSleepers, ScalabilityHint::Few, Options);
		PET_EXTRACT_TYPE(Profile, HardwareProfile, void, Options);
		PET_EXTRACT_TEMPLATE(PolicyTemplate, SchedulingPolicy, RoundRobinPolicy, Options);

	public:
		/*
		 * Public, front-end types
		 */
		using TickType = typename Profile::TickType;
		class ErrorStrings;

		template<class Data> class Atomic;

		class Task;
		class Mutex;
		class SharedAtomicList;
		class BinarySemaphore;
		class CountingSemaphore;

		class IoChannel;
		template<class> class IoRequest;

	private:
		template<class> friend class OsInternalTester;
		/*
		 * Internal types.
		 */
		class Blockable;
		class Blocker;
		class SharedBlocker;
		template<class> class AsyncBlocker;
		template<class> class WaitableBlocker;
		template<class> class SemaphoreLikeBlocker;
		class WaitableSet;

		class IoRequestCommon;

		class Policy;
		template<ScalabilityHint, class = void> class SleeperBase;
		template<ScalabilityHint, class = void> class SleepListBase;

		class Event;
		class Timeout;
		class EventList;
		class PreemptionEvent;

		template<bool, class=void> class AssertSwitch;
		template<class, bool> class ObjectRegistry;
		template<class...> class RegistryRootHub;
		struct SyscallMap;


		/*
		 * Helper type and template aliases.
		 */
		template<class Object> using Registry = ObjectRegistry<Object, registryEnabled>;
		using PolicyBase = PolicyTemplate<Task, Blockable>;
		using Sleeper = class SleeperBase<sleeperStorageOption>;
		using SleepList = class SleepListBase<sleeperStorageOption>;

		/*
		 * Definitions of system calls.
		 */
		static void onTick();
		static uintptr_t doStartTask(uintptr_t task);
		static uintptr_t doExit();
		static uintptr_t doYield();
		static uintptr_t doAbort(uintptr_t errorMessage);
		static uintptr_t doSleep(uintptr_t time);
		template<class> static uintptr_t doRegisterObject(uintptr_t object);
		template<class> static uintptr_t doUnregisterObject(uintptr_t object);
		template<class> static uintptr_t doBlock(uintptr_t blocker);
		template<class> static uintptr_t doTimedBlock(uintptr_t blocker, uintptr_t time);
		template<class> static uintptr_t doRelease(uintptr_t blocker, uintptr_t arg);

		template<class Syscall, class... T> static uintptr_t syscall(T...);
		template<class Syscall, class... T> static inline uintptr_t conditionalSyscall(T... );

		static inline void assert(bool, const char*);

		template<class T> static inline void resetObject(T*);

		/**
		 * Task switching helper
		 */
		template<bool pendOld, bool suspend = true> static inline void switchToNext();

		/**
		 * Callback for platform implementation that returns the system call
		 * method to be called according to the parameter passed (generated
		 * by _syscall_ helper method via the _SyscallMap_).
		 */
		static inline void* syscallMapper(uintptr_t);

		/**
		 * Internal helper to get the currently executing task or null if suspended.
		 */
		static inline Task* getCurrentTask();

		/**
		 * The globally visible internal state wrapped in a single struct.
		 */
		static struct State: RegistryRootHub<
				Mutex,
				CountingSemaphore,
				BinarySemaphore,
				WaitableSet,
				IoChannel,
				IoRequestCommon
			> {
			inline void* operator new(size_t, void* x) { return x; }
			Policy policy;
			bool isRunning = false;
			uintptr_t nTasks = 0;
			EventList eventList;
			SleepList sleepList;
			PreemptionEvent preemptionEvent;
		} state;
	public:
		/**
		 * Get current low resolution time.
		 */
		inline static TickType getTick();

		/**
		 * Start scheduler with platform dependent parameters.
		 */
		template<class... T> inline static const char* start(T... t);

		/**
		 * Abort scheduler and return to starting point with value (from task).
		 */
		inline static void abort(const char*);

		/**
		 * Give up task execution momentarily.
		 */
		inline static void yield();

		/**
		 * Give up task execution for some minimal time.
		 */
		inline static void sleep(uintptr_t time);

		/**
		 * Give up task execution permanently, exit task.
		 */
		inline static void exit();

		/**
		 * Wait for any of the targets.
		 */
		template<class... T> inline static Blocker* select(T... t);

		/**
		 * Wait for any of the targets with timeout.
		 */
		template<class... T> inline static Blocker* selectTimeout(uintptr_t timout, T... t);
	};
};

template<class... Args>
using Scheduler = SchedulerOptions::Configurable<Args...>;

#include "internal/Atomic.h"
#include "internal/SharedAtomicList.h"
#include "internal/Event.h"
#include "internal/Timeout.h"
#include "internal/EventList.h"
#include "internal/Preemption.h"

#include "syscall/Helpers.h"
#include "syscall/ErrorStrings.h"
#include "syscall/Syscall.h"
#include "syscall/ObjectRegistry.h"

#include "blocking/Policy.h"

#include "blocking/Blocker.h"
#include "blocking/AsyncBlocker.h"
#include "blocking/SharedBlocker.h"
#include "blocking/WaitableBlocker.h"
#include "blocking/SemaphoreLikeBlocker.h"

#include "blocking/Sleeping.h"
#include "blocking/Blockable.h"
#include "blocking/WaitableSet.h"

#include "frontend/Mutex.h"
#include "frontend/Task.h"
#include "frontend/BinarySemaphore.h"
#include "frontend/CountingSemaphore.h"
#include "frontend/IoRequest.h"
#include "frontend/IoChannel.h"

///////////////////////////////////////////////////////////////////////////////

template<class... Args>
typename Scheduler<Args...>::State Scheduler<Args...>::state;

template<class... Args>
template<class... T>
inline const char* Scheduler<Args...>::start(T... t) {
	/*
	 * Initialize internal state.
	 */
	Task* firstTask = state.policy.popNext();
	state.isRunning = true;

	/*
	 * Initialize platform.
	 */
	Profile::setTickHandler(&Scheduler<Args...>::onTick);
	Profile::setSyscallMapper(&syscallMapper);
	Profile::init(t...);

	/*
	 * Start scheduling.
	 */
	const char* ret = Profile::startFirst(firstTask);

	/*
	 * Reset handlers to detect platform tear-down issues.
	 */
	Profile::setTickHandler(nullptr);
	Profile::setSyscallMapper(nullptr);

	/*
	 * Reset state to initial values.
	 */
	state.~State();
	new(&state) State();

	return ret;
}

template<class... Args>
inline typename Scheduler<Args...>::Profile::TickType Scheduler<Args...>::getTick() {
	return Profile::getTick();
}

template<class... Args>
inline void Scheduler<Args...>::abort(const char* errorMessage) {
	syscall<SYSCALL(doAbort)>(reinterpret_cast<uintptr_t>(errorMessage));
}

template<class... Args>
inline uintptr_t Scheduler<Args...>::doAbort(uintptr_t errorMessage) {
	Profile::finishLast(reinterpret_cast<const char*>(errorMessage));
	return errorMessage;
}

#endif /* SCHEDULER_H_ */
