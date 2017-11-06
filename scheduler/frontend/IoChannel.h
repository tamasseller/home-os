/*
 * IoChannel.h
 *
 *  Created on: 2017.11.05.
 *      Author: tooma
 */

#ifndef IOCHANNEL_H_
#define IOCHANNEL_H_

#include "Scheduler.h"

template<class... Args>
class Scheduler<Args...>::IoChannel {
	friend Scheduler<Args...>;

	virtual void enableProcess() = 0;
	virtual void disableProcess() = 0;

public:
	class Job: Sleeper, Event {
	public:

		enum class Result: uintptr_t{
			Done, Canceled, TimedOut
		};

	private:
		friend Scheduler<Args...>;
		friend pet::DoubleList<Job>;

		Job *next, *prev;
		IoChannel * volatile channel = nullptr;
		void (* const finished)(Job*, Result);

		static constexpr intptr_t submitNoTimeoutValue = (intptr_t) -1;
		static constexpr intptr_t cancelValue = (intptr_t) -2;
		static constexpr intptr_t doneValue = (intptr_t) -3;

		static void handleRequest(Event* event, uintptr_t uarg)
		{
			Job* self = static_cast<Job*>(event);
			IoChannel *channel = self->channel;
			intptr_t arg = (intptr_t)uarg;

			bool hasTimeout = arg > 0;
			bool isSubmit = hasTimeout || arg == submitNoTimeoutValue;

			if(isSubmit) {

				// assert(channel, "Internal error, invalid I/O operation state");

				channel->disableProcess();
				channel->requests.addBack(self);
				channel->enableProcess();

				if(hasTimeout) {
					// assert(!self->isSleeping(), "Internal error, invalid I/O operation state");
					state.sleepList.delay(self, arg);
				}
			} else {
				bool isCancel = hasTimeout || arg == cancelValue;
				bool isDone = hasTimeout || arg == doneValue;

				// assert(isDone || isCancel, "Internal error, invalid I/O operation argument");

				if(self->isSleeping())
					state.sleepList.remove(self);

				if(isCancel) {

					// assert(channel, "Internal error, invalid I/O operation state");

					channel->disableProcess();

					bool ok = channel->requests.remove(self);
					//assert(ok, "Internal error, invalid I/O operation state");

					self->channel = nullptr;

					if(channel->requests.front())
						channel->enableProcess();

					self->finished(self, Result::Canceled);
				} else
					self->finished(self, Result::Done);

			}
		}

		static void timedOut(Sleeper* sleeper) {
			Job* self = static_cast<Job*>(sleeper);

			if(IoChannel *channel = self->channel) {
				channel->disableProcess(); // TODO think more about this race condition.

				channel->requests.remove(self);
				self->channel = nullptr;

				if(channel->requests.front())
					channel->enableProcess();

				self->finished(self, Result::TimedOut);
			}
		}

		static inline void nop(Job* job, Result result) {}

	public:

		inline Job(void (*finished)(Job*, Result) = &Job::nop):
			Sleeper(&Job::timedOut),
			Event(&Job::handleRequest),
			finished(finished) {}
	};

private:

	pet::DoubleList<Job> requests;

	template<uintptr_t value>
	struct OverwriteCombiner {
		inline bool operator()(uintptr_t old, uintptr_t& result) const {
			result = value;
			return true;
		}
	};

protected:

	Job* currentJob() {
		return requests.front();
	}

	void jobDone() {
		Job* job = requests.popFront();

		if(!requests.front())
			disableProcess();

		job->channel = nullptr;

		state.eventList.issue(job, OverwriteCombiner<Job::doneValue>());
	}

public:

	bool submit(Job* job)
	{
		if(job->channel)
			return false;

		job->channel = this;
		state.eventList.issue(job, OverwriteCombiner<Job::submitNoTimeoutValue>());
		return true;
	}

	bool submitTimeout(Job* job, uintptr_t time)
	{
		if(job->channel || !time || time >= (uintptr_t)INTPTR_MAX)
			return false;

		job->channel = this;
		state.eventList.issue(job, [time](uintptr_t, uintptr_t& result) {
			result = time;
			return true;
		});

		return true;
	}

	bool cancel(Job* job)
	{
		if(!job->channel)
			return false;

		state.eventList.issue(job, OverwriteCombiner<Job::cancelValue>());
		return true;
	}
};

#endif /* IOCHANNEL_H_ */
