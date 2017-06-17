/*
 * Preemption.h
 *
 *  Created on: 2017.06.11.
 *      Author: tooma
 */

#ifndef PREEMPTION_H_
#define PREEMPTION_H_

template<class... Args>
class Scheduler<Args...>::PreemptionEvent: public Event {
	static inline void execute(Event* self, uintptr_t arg) {
		assert(arg == 1, "Tick overload, preemption event could not have been dispatched for a full tick cycle!");

		while(Sleeper* sleeper = state.sleepList.getWakeable()) {
			Task* task = static_cast<Task*>(sleeper);
			if(task->blockedBy) {
				task->blockedBy->remove(task);
				task->blockedBy = nullptr;
			}
			state.policy.addRunnable(task);
		}

		if(Task* newTask = state.policy.peekNext()) {
			if(typename Profile::Task* platformTask = Profile::Task::getCurrent()) {

				Task* currentTask = static_cast<Task*>(platformTask);

				if(firstPreemptsSecond(currentTask, newTask))
					return;

				state.policy.addRunnable(currentTask);
			}

			state.policy.popNext();
			static_cast<Task*>(newTask)->switchToAsync();
		}
	}

public:
	inline PreemptionEvent(): Event(execute) {}
};

template<class... Args>
void Scheduler<Args...>::onTick()
{
	state.eventList.issue(&state.preemptionEvent);
}

#endif /* PREEMPTION_H_ */
