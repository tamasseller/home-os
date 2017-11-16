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

    struct Reactivator {
        virtual bool reactivate(IoJob*, IoChannel*) const = 0;
        virtual bool reactivateTimeout(IoJob*, IoChannel*, uintptr_t) const = 0;
    };

private:

    class DefaultReactivator: public Reactivator {
        virtual bool reactivate(IoJob* job, IoChannel* channel) const override final {
            return channel->submitPrepared(job);
        }

        virtual bool reactivateTimeout(IoJob* job, IoChannel* channel, uintptr_t timeout) const override final {
            return channel->submitTimeoutPrepared(job, timeout);
        }
    };


public /* IoChannel implementations */:
    IoJob *next, *prev;
	uintptr_t param;


private:
	// TODO describe actors and their locking and actions.
	Atomic<IoChannel *> channel = nullptr;

	bool (* finished)(IoJob*, Result, const Reactivator&);


	static constexpr intptr_t submitNoTimeoutValue = (intptr_t) -1;
	static constexpr intptr_t cancelValue = (intptr_t) -2;
	static constexpr intptr_t doneValue = (intptr_t) -3;

	static inline bool removeSynhronized(IoJob* self) {
		// TODO describe locking hackery (race between process completion (and
		// possible re-submission), cancelation and timeout on channel field).
		while(IoChannel *channel = self->channel) {

			Registry<IoChannel>::check(channel);

			channel->disableProcess();

			Profile::memoryFence();

			if(channel != self->channel) {
				channel->enableProcess();
				continue;
			}

			bool ok = channel->removeJob(self);
			/*
			 * This operation must not fail because the channel pointer
			 * is acquired exclusively
			 */
			assert(ok, "Internal error, invalid I/O operation state");

			self->channel = nullptr;

			if(channel->hasJob())
				channel->enableProcess();

			self->finished(self, Result::Canceled, DefaultReactivator());
			return ok;
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

			assert(channel, "Internal error, invalid I/O operation state");

			Registry<IoChannel>::check(channel);

			channel->disableProcess();
			channel->addJob(self);
			channel->enableProcess();

			if(hasTimeout) {
				if(self->isSleeping())
					state.sleepList.update(self, arg);
				else
					state.sleepList.delay(self, arg);
			}
		} else {
			bool isCancel = hasTimeout || arg == cancelValue;
			bool isDone = hasTimeout || arg == doneValue;

			assert(isDone || isCancel, "Internal error, invalid I/O operation argument");

			if(self->isSleeping())
				state.sleepList.remove(self);

			if(isCancel) {
				if(removeSynhronized(self))
					self->finished(self, Result::Canceled, DefaultReactivator());
			} else
				self->finished(self, Result::Done, DefaultReactivator());

		}
	}

	static void timedOut(Sleeper* sleeper) {
		IoJob* self = static_cast<IoJob*>(sleeper);

		if(removeSynhronized(self))
			self->finished(self, Result::TimedOut, DefaultReactivator());
	}

protected:
	inline void prepare(bool (*finished)(IoJob*, Result, const Reactivator &), uintptr_t param = 0) {
		this->param = param;
		this->finished = finished;
	}

public:

	inline IoJob(): Sleeper(&IoJob::timedOut), Event(&IoJob::handleRequest) {}
};

}

#endif /* IOJOB_H_ */
