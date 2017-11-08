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
class Scheduler<Args...>::IoChannel: Registry<IoChannel>::ObjectBase { // TODO add registration
	friend Scheduler<Args...>;

public:
	class Job: Sleeper, Event {
	public:

		enum class Result: uintptr_t{
			Done, Canceled, TimedOut
		};

	private:
		friend Scheduler<Args...>;

		// TODO describe actors and their locking and actions.
		Atomic<IoChannel *> channel = nullptr;

		void (* const finished)(Job*, Result);

		static constexpr intptr_t submitNoTimeoutValue = (intptr_t) -1;
		static constexpr intptr_t cancelValue = (intptr_t) -2;
		static constexpr intptr_t doneValue = (intptr_t) -3;

		static inline bool removeSynhronized(Job* self) {
			// TODO describe locking hackery (race between process completion (and possible re-submission),
			// cancelation and timeout on channel field).
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

				self->finished(self, Result::Canceled);
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
						self->finished(self, Result::Canceled);
				} else
					self->finished(self, Result::Done);

			}
		}

		static void timedOut(Sleeper* sleeper) {
			Job* self = static_cast<Job*>(sleeper);

			if(removeSynhronized(self))
				self->finished(self, Result::TimedOut);
		}

		static inline void nop(Job* job, Result result) {}

	public:

		inline Job(void (*finished)(Job*, Result) = &Job::nop):
			Sleeper(&Job::timedOut),
			Event(&Job::handleRequest),
			finished(finished) {}
	};

private:

	virtual void enableProcess() = 0;
	virtual void disableProcess() = 0;
	virtual bool hasJob() = 0;
	virtual bool addJob(Job*) = 0;
	virtual bool removeJob(Job*) = 0;

	template<uintptr_t value>
	struct OverwriteCombiner {
		inline bool operator()(uintptr_t old, uintptr_t& result) const {
			result = value;
			return true;
		}
	};

protected:
	void jobDone(Job* job) {
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

private:

	inline bool takeJob(Job* job) {
		return job->channel.compareAndSwap(nullptr, this);
	}

public:

	void init() {
		Registry<IoChannel>::registerObject(this);
	}

	bool submit(Job* job)
	{
		if(!takeJob(job))
			return false;

		state.eventList.issue(job, OverwriteCombiner<Job::submitNoTimeoutValue>());
		return true;
	}

	bool submitTimeout(Job* job, uintptr_t time)
	{
		if(!time || time >= (uintptr_t)INTPTR_MAX)
			return false;

		if(!takeJob(job))
			return false;

		state.eventList.issue(job, [time](uintptr_t, uintptr_t& result) {
			result = time;
			return true;
		});

		return true;
	}

	void cancel(Job* job)
	{
		state.eventList.issue(job, OverwriteCombiner<Job::cancelValue>());
	}

	~IoChannel() {
		/*
		 * Io channels must be deleted very carefully, because there
		 * can be jobs referencing it that can trigger execution in
		 * various asynchronous contexts.
		 */

		assert(!state.isRunning, ErrorStrings::ioChannelDelete);
	}
};

#endif /* IOCHANNEL_H_ */
