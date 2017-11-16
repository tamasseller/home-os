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

#ifndef IOCHANNEL_H_
#define IOCHANNEL_H_

#include "Scheduler.h"

namespace home {

template<class... Args>
class Scheduler<Args...>::IoChannel: Registry<IoChannel>::ObjectBase { // TODO add registration
	friend Scheduler<Args...>;

public:
	class Job;

private:
	virtual void enableProcess() = 0;
	virtual void disableProcess() = 0;
	virtual bool addJob(Job*) = 0;
	virtual bool removeJob(Job*) = 0;

	template<uintptr_t value>
	struct OverwriteCombiner {
		inline bool operator()(uintptr_t old, uintptr_t& result) const {
			result = value;
			return true;
		}
	};

    inline bool takeJob(Job* job);

	inline bool hasJob() {
		return jobs.front() != nullptr;
	}

	inline bool submitPrepared(Job* job);
	inline bool submitTimeoutPrepared(Job* job, uintptr_t time);

protected:
	pet::DoubleList<Job> jobs;

	void jobDone(Job* job);

public:
	template<class ActualJob, class... C>
	inline bool submit(ActualJob* job, C... c);

	template<class ActualJob, class... C>
	inline bool submitTimeout(ActualJob* job, uintptr_t time, C... c);

	void cancel(Job* job);

	void init();
	~IoChannel();
};

template<class... Args>
inline bool Scheduler<Args...>::IoChannel::takeJob(Job* job) {
	return job->channel.compareAndSwap(nullptr, this);
}

template<class... Args>
inline void Scheduler<Args...>::IoChannel::jobDone(Job* job) {
	/*
	 * TODO describe relevance in locking mechanism of removeSynchronized.
	 */
	job->channel = nullptr;

	Profile::memoryFence();

	removeJob(job);

	if(!hasJob())
		disableProcess();

	state.eventList.issue(job, OverwriteCombiner<Job::doneValue>());
}

template<class... Args>
inline void Scheduler<Args...>::IoChannel::init() {
	Registry<IoChannel>::registerObject(this);
}

template<class... Args>
template<class ActualJob, class... C>
inline bool Scheduler<Args...>::IoChannel::submit(ActualJob* job, C... c)
{
	if(!takeJob(job))
		return false;

	job->prepare(c...);

	state.eventList.issue(static_cast<Job*>(job), OverwriteCombiner<Job::submitNoTimeoutValue>());
	return true;
}

template<class... Args>
template<class ActualJob, class... C>
inline bool Scheduler<Args...>::IoChannel::submitTimeout(ActualJob* job, uintptr_t time, C... c)
{
	if(!time || time >= (uintptr_t)INTPTR_MAX)
		return false;

	if(!takeJob(job))
		return false;

	job->prepare(c...);

	state.eventList.issue(static_cast<Job*>(job), [time](uintptr_t, uintptr_t& result) {
		result = time;
		return true;
	});

	return true;
}

template<class... Args>
inline bool Scheduler<Args...>::IoChannel::submitPrepared(Job* job)
{
	if(!takeJob(job))
		return false;

	state.eventList.issue(static_cast<Job*>(job), OverwriteCombiner<Job::submitNoTimeoutValue>());
	return true;
}

template<class... Args>
inline bool Scheduler<Args...>::IoChannel::submitTimeoutPrepared(Job* job, uintptr_t time)
{
	if(!time || time >= (uintptr_t)INTPTR_MAX)
		return false;

	if(!takeJob(job))
		return false;

	state.eventList.issue(static_cast<Job*>(job), [time](uintptr_t, uintptr_t& result) {
		result = time;
		return true;
	});

	return true;
}

template<class... Args>
inline void Scheduler<Args...>::IoChannel::cancel(Job* job)
{
	state.eventList.issue(job, OverwriteCombiner<Job::cancelValue>());
}

template<class... Args>
inline Scheduler<Args...>::IoChannel::~IoChannel() {
	/*
	 * Io channels must be deleted very carefully, because there
	 * can be jobs referencing it that can trigger execution in
	 * various asynchronous contexts.
	 */

	assert(!state.isRunning, ErrorStrings::ioChannelDelete);
}

/**
 * I/O work item.
 *
 * TODO describe in detal
 */
template<class... Args>
class Scheduler<Args...>::IoChannel::Job: Sleeper, Event {
	friend Scheduler<Args...>;

public:

	enum class Result: uintptr_t{
		NotYet, Done, Canceled, TimedOut
	};

    struct Reactivator {
        virtual bool reactivate(Job*, IoChannel*) const = 0;
        virtual bool reactivateTimeout(Job*, IoChannel*, uintptr_t) const = 0;
    };

private:

    class DefaultReactivator: public Reactivator {
        virtual bool reactivate(Job* job, IoChannel* channel) const override final {
            return channel->submitPrepared(job);
        }

        virtual bool reactivateTimeout(Job* job, IoChannel* channel, uintptr_t timeout) const override final {
            return channel->submitTimeoutPrepared(job, timeout);
        }
    };


public /* IoChannel implementations */:
    Job *next, *prev;
	uintptr_t param;


private:
	// TODO describe actors and their locking and actions.
	Atomic<IoChannel *> channel = nullptr;

	bool (* finished)(Job*, Result, const Reactivator&);


	static constexpr intptr_t submitNoTimeoutValue = (intptr_t) -1;
	static constexpr intptr_t cancelValue = (intptr_t) -2;
	static constexpr intptr_t doneValue = (intptr_t) -3;

	static inline bool removeSynhronized(Job* self) {
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
		Job* self = static_cast<Job*>(event);
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
		Job* self = static_cast<Job*>(sleeper);

		if(removeSynhronized(self))
			self->finished(self, Result::TimedOut, DefaultReactivator());
	}

protected:
	inline void prepare(bool (*finished)(Job*, Result, const Reactivator &), uintptr_t param = 0) {
		this->param = param;
		this->finished = finished;
	}

public:

	inline Job(): Sleeper(&Job::timedOut), Event(&Job::handleRequest) {}
};


}

#endif /* IOCHANNEL_H_ */
