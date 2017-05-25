/*
 * Scheduler.h
 *
 *  Created on: 2017.05.24.
 *      Author: tooma
 */

#ifndef SCHEDULER_H_
#define SCHEDULER_H_


template<class Profile, class Policy>
class Scheduler {
	static Policy policy;

	template<class T>
	static uintptr_t detypePtr(T* x) {
		return reinterpret_cast<uintptr_t>(x);
	}

	template<class T>
	static T* entypePtr(uintptr_t  x) {
		return reinterpret_cast<T*>(x);
	}

public:
	class TaskBase: Profile::Task, Policy::Task {
		friend Scheduler;
	};

	template<class Child>
	class Task: public TaskBase {

	public:
		inline Task(void* stack, uint32_t stackSize) {
			Profile::Task::template initialize<
				Child,
				&Child::run,
				&Scheduler::exit
			> (stack, stackSize, static_cast<Child*>(this));
		}

		inline void start() {
			Profile::CallGate::sync(&Scheduler::doStartTask, detypePtr(static_cast<TaskBase*>(this)));
		}
	};
protected:
	static TaskBase* currentTask;

	inline static bool switchToNext()
	{
		if(TaskBase* newTask = static_cast<TaskBase*>(policy.getNext())) {
			currentTask->switchTo(newTask);
			currentTask = newTask;
			return true;
		}

		return false;
	}

	static uintptr_t doStartTask(uintptr_t task) {
		policy.addRunnable(entypePtr<TaskBase>(task));
	}

	static uintptr_t doExit() {
		if(!switchToNext())
			currentTask->finishLast();
	}

	static uintptr_t doYield() {
		policy.addRunnable(currentTask);
		switchToNext();
	}
public:
	static void start() {
		currentTask = static_cast<TaskBase*>(policy.getNext());
		currentTask->startFirst();
	}

	static typename Profile::Timer::TickType getTick() {
		return Profile::Timer::getTick();
	}

	inline static void yield() {
		Profile::CallGate::sync(&Scheduler::doYield);
	}

	inline static void exit() {
		Profile::CallGate::sync(&Scheduler::doExit);
	}
};

template<class Profile, class Policy>
class SchedulerInternalAccessor: private Scheduler<Profile, Policy> {
protected:
	using Scheduler<Profile, Policy>::selectNext;
};

template<class Profile, class Policy>
Policy Scheduler<Profile, Policy>::policy;

template<class Profile, class Policy>
typename Scheduler<Profile, Policy>::TaskBase* Scheduler<Profile, Policy>::currentTask;


#endif /* SCHEDULER_H_ */
