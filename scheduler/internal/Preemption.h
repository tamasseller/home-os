/*******************************************************************************
 *
 * Copyright (c) 2017 Seller Tamás. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *******************************************************************************/

#ifndef PREEMPTION_H_
#define PREEMPTION_H_

namespace home {

template<class... Args>
class Scheduler<Args...>::PreemptionEvent: public Event {
	static inline void execute(Event* self, uintptr_t arg) {
		assert(arg == 1, ErrorStrings::interruptOverload);

		while(Sleeper* sleeper = state.sleepList.getWakeable())
			sleeper->wake();

		if(Task* newTask = state.policy.peekNext()) {
			if(typename Profile::Task* platformTask = Profile::getCurrent()) {

				Task* currentTask = static_cast<Task*>(platformTask);

				if(currentTask->priority < newTask->priority)
					return;

				state.policy.addRunnable(currentTask);
			}

			state.policy.popNext();
			Profile::switchToAsync(newTask);
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

}

#endif /* PREEMPTION_H_ */
