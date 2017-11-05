/*
 * Timeout.h
 *
 *  Created on: 2017.11.05.
 *      Author: tooma
 */

#ifndef TIMEOUT_H_
#define TIMEOUT_H_

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
			if(self->isSleeping())
				state.sleepList.update(self, arg);
			else
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


#endif /* TIMEOUT_H_ */
