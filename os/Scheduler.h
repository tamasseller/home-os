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

public:
	class Task: Profile::Task, Policy::Task {
		friend Scheduler;

		static void entry(void* self) {
			((Task*)self)->run();
		}

		static void exit() {}

		virtual void run() = 0;
	public:
		inline Task(void* stack, uint32_t stackSize) {
			Profile::Task::template initialize<&Task::entry, &Task::exit>(stack, stackSize, this);
		}

		inline void start() {
			startTask(this);
		}
	};
protected:
	static Task* currentTask;

	static void startTask(Task* task) {
		policy.addRunnable(task);
	}

	inline static void switchToNext() {
		Task* oldTask = currentTask;
		currentTask = static_cast<Task*>(policy.getNext());
		oldTask->switchTo(currentTask);
	}

	static uintptr_t doYield() {
		policy.addRunnable(currentTask);
		switchToNext();
	}
public:
	static void start() {
		currentTask = static_cast<Task*>(policy.getNext());
		currentTask->startFirst();
	}

	static typename Profile::Timer::TickType getTick() {
		return Profile::Timer::getTick();
	}

	inline static void yield() {
		Profile::CallGate::issue(&Scheduler::doYield);
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
typename Scheduler<Profile, Policy>::Task* Scheduler<Profile, Policy>::currentTask;


#endif /* SCHEDULER_H_ */
