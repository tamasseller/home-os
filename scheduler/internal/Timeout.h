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

#ifndef TIMEOUT_H_
#define TIMEOUT_H_

namespace home {

template<class... Args>
class Scheduler<Args...>::Timeout: Sleeper, Event {
	friend Scheduler<Args...>;

	static void modify(Event* event, uintptr_t arg)
	{
		Timeout* self = static_cast<Timeout*>(event);

		if(arg == (uintptr_t)-1) {
			if(self->isSleeping())
				state.sleepList.remove(self);
		} else {
			state.sleepList.delay(self, arg);
		}
	}

public:
	inline Timeout(void (*wake)(Sleeper* sleeper)): Sleeper(wake), Event(&Timeout::modify) {}

	void start(uintptr_t time) {
		if(time && time < INTPTR_MAX) {
			state.eventList.issue(this, [time](uintptr_t, uintptr_t& result) {
				result = time;
				return true;
			});
		}
	}

	void cancel() {
		state.eventList.issue(this, [](uintptr_t, uintptr_t& result) {
			result = (uintptr_t)-1;
			return true;
		});
	}
};

}

#endif /* TIMEOUT_H_ */
