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

#ifndef IOJOB_H_
#define IOJOB_H_

namespace home {

/**
 * I/O work item.
 *
 * TODO describe in detal
 */
template<class... Args>
class Scheduler<Args...>::IoJob: Sleeper, Event {
	friend Scheduler<Args...>;

public:

	enum class Result: uintptr_t{
		NotYet, Done, Canceled, TimedOut
	};

	typedef void (*Hook)(IoJob*);
	typedef bool (*Callback)(IoJob*, Result, Hook);

	class Data {
		friend Scheduler<Args...>;
		IoJob* job;
	};

private:
	// TODO describe actors and their locking and actions.
	Atomic<IoChannel *> channel = nullptr;
	Data* data;
	Callback finished;

	static constexpr intptr_t submitNoTimeoutValue = (intptr_t) -1;
	static constexpr intptr_t cancelValue = (intptr_t) -2;
	static constexpr intptr_t doneValue = (intptr_t) -3;

	inline void removeSynhronized(Result result) {
		// TODO describe locking hackery (race between process completion (and
		// possible re-submission), cancelation and timeout on channel field).
		while(IoChannel *channel = this->channel) {

			Registry<IoChannelCommon>::check(static_cast<IoChannelCommon*>(channel));

			typename IoChannel::RemoveResult removeResult = channel->remove(this);
			if(removeResult == IoChannel::RemoveResult::Raced)
				continue;

			/*
			 * This operation must not fail because the channel pointer
			 * is acquired exclusively
			 */
			assert(removeResult == IoChannel::RemoveResult::Ok, ErrorStrings::ioRequestState);

			finished(this, result, nullptr);
			break;
		}
	}

	static void handleRequest(Event* event, uintptr_t uarg)
	{
		IoJob* self = static_cast<IoJob*>(event);
		intptr_t arg = (intptr_t)uarg;

		bool hasTimeout = arg > 0;
		bool isSubmit = hasTimeout || arg == submitNoTimeoutValue;

		if(isSubmit) {
			/*
			 * At this point channel should carry no race condition because:
			 *
			 *  - The submit methods acquire the channel field atomically
			 *    if it was found to be null, thus they implementing mutual
			 *    exclusion among them and the process.
			 *  - The channel field is known to have contained null before the
			 *    submit request was issued, the Job can not be present in the
			 *    queue of the process, meaning that it can not see and reset
			 *    the channel field.
			 *  - The cancellation event and the time out handler - the last
			 *    actors that - can modify the field are synchronized with the
			 *    submission handlers implicitly.
			 */
			IoChannel *channel = self->channel;

			assert(channel, ErrorStrings::ioRequestState);

			Registry<IoChannelCommon>::check(static_cast<IoChannelCommon*>(channel));

			channel->add(self);

			if(hasTimeout) {
				if(self->isSleeping())
					state.sleepList.update(self, arg);
				else
					state.sleepList.delay(self, arg);
			}
		} else {
			bool isCancel = hasTimeout || arg == cancelValue;
			bool isDone = hasTimeout || arg == doneValue;

			assert(isDone || isCancel, ErrorStrings::ioRequestArgument);

			if(self->isSleeping())
				state.sleepList.remove(self);

			if(isCancel) {
				self->removeSynhronized(Result::Canceled);
			} else
				self->finished(self, Result::Done, nullptr);

		}
	}

	static void timedOut(Sleeper* sleeper) {
		static_cast<IoJob*>(sleeper)->removeSynhronized(Result::TimedOut);
	}

	template<uintptr_t value> struct OverwriteCombiner {
		inline bool operator()(uintptr_t old, uintptr_t& result) const {
			result = value;
			return true;
		}
	};

	inline bool takeOver(void (*hook)(IoJob*), IoChannel* channel, Callback callback, Data* data) {
		if(!this->channel.compareAndSwap(nullptr, channel))
			return false;

		this->finished = callback;
		data->job = this;
		this->data = data;

		if(hook)
			hook(this);

		return true;
    }

protected:

	inline bool submit(void (*hook)(IoJob*), IoChannel* channel, Callback callback, Data* data) {
		if(!takeOver(hook, channel, callback, data))
			return false;

		state.eventList.issue(this, OverwriteCombiner<IoJob::submitNoTimeoutValue>());
		return true;
	}

	inline bool submitTimeout(Hook hook, IoChannel* channel, uintptr_t time, Callback callback, Data* data)
	{
		if(!time || time >= (uintptr_t)INTPTR_MAX)
			return false;

		if(!takeOver(hook, channel, callback, data))
			return false;

		state.eventList.issue(this, [time](uintptr_t, uintptr_t& result) {
			result = time;
			return true;
		});

		return true;
	}

public:

	inline IoJob(): Sleeper(&IoJob::timedOut), Event(&IoJob::handleRequest) {}

	inline void cancel() {
		state.eventList.issue(this, OverwriteCombiner<IoJob::cancelValue>());
	}

};

}

#endif /* IOJOB_H_ */
