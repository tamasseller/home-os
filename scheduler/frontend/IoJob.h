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

	class Data {
		friend Scheduler<Args...>;
		IoJob* job;
	};

	enum class Result: uintptr_t{
		NotYet, Done, Canceled, TimedOut
	};

	class Launcher;
	typedef bool (*Callback)(Launcher*, IoJob*, Result);

protected:
	template<uintptr_t value> struct OverwriteCombiner;
	struct ParamOverwriteCombiner;

private:
	// TODO describe actors and their locking and actions.
	Atomic<IoChannel *> channel = nullptr;
	Data* data;
	Callback finished;

	class NormalLauncher;
	class TimeoutLauncher;

	static constexpr intptr_t submitNoTimeoutValue = (intptr_t) -1;
	static constexpr intptr_t cancelValue = (intptr_t) -2;
	static constexpr intptr_t doneValue = (intptr_t) -3;

	inline void removeSynhronized(Result result) {
		// TODO describe locking hackery (race between process completion (and
		// possible re-submission), cancelation and timeout on channel field).
		while(IoChannel *channel = this->channel) {
			Registry<IoChannel>::check(channel);

			typename IoChannel::RemoveResult removeResult = channel->remove(this, result);
			if(removeResult == IoChannel::RemoveResult::Raced)
				continue;

			if(removeResult != IoChannel::RemoveResult::Denied) {
				NormalLauncher launcher(this);
				finished(&launcher, this, result);

				if(this->isSleeping())
					state.sleepList.remove(this);
			}

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

			Registry<IoChannel>::check(channel);

			channel->add(self);

			if(hasTimeout)
				state.sleepList.delay(self, arg);

		} else {
			if(arg == cancelValue) {
				self->removeSynhronized(Result::Canceled);
			} else if(arg == doneValue){
			    NormalLauncher launcher(self);
				bool goOn = self->finished(&launcher, self, Result::Done);

				if(!goOn) {
					if(self->isSleeping())
						state.sleepList.remove(self);

					self->channel = nullptr;
				}
			} else
				assert(false, ErrorStrings::ioRequestArgument); // Never executed normally ( LCOV_EXCL_LINE ).

		}
	}

	static void timedOut(Sleeper* sleeper) {
		static_cast<IoJob*>(sleeper)->removeSynhronized(Result::TimedOut);
	}

	bool acquire() {
	    return channel.compareAndSwap(nullptr, reinterpret_cast<IoChannel*>(-1));
	}

	template<class Method, class... C>
	bool launchWithLauncher(Launcher* launcher, Method method, C... c) {
	        if(!acquire())
	            return false;

	        if(!method(launcher, this, c...)) {
	            channel = nullptr;
	            return false;
	        }

	        return true;
	}

public:
	inline IoJob(): Sleeper(&IoJob::timedOut), Event(&IoJob::handleRequest) {}

	inline bool isOccupied() {
		return this->channel != nullptr;
	}

	template<class Method, class... C>
	inline bool launch(Method method, C... c) {
        NormalLauncher launcher(this);
        return launchWithLauncher(&launcher, method, c...);
	}

	template<class Method, class... C>
	inline bool launchTimeout(Method method, uintptr_t time, C... c)
	{
        if(!time || time >= (uintptr_t)INTPTR_MAX)
            return false;

		TimeoutLauncher launcher(this, time);
		return launchWithLauncher(&launcher, method, c...);
	}

	inline void cancel() {
		state.eventList.issue(this, OverwriteCombiner<IoJob::cancelValue>());
	}

	/**
	 * Deleted copy constructors and assignment operators.
	 *
	 * IoJobs should be stable and uniquely referenceable entities
	 * throughout their whole liftime, because the references to them
	 * are used in scenarios where race conditions pose tight restrictions
	 * on the runtime sanity checks that can be performed on them.
	 */
	IoJob(const IoJob&) = delete;
	IoJob(IoJob&&) = delete;
	void operator =(const IoJob&) = delete;
	void operator =(IoJob&&) = delete;
};

template<class... Args>
template<uintptr_t value>
struct Scheduler<Args...>::IoJob::OverwriteCombiner {
	inline bool operator()(uintptr_t old, uintptr_t& result) const {
		result = value;
		return true;
	}
};

template<class... Args>
struct Scheduler<Args...>::IoJob::ParamOverwriteCombiner {
	uintptr_t value;

	inline bool operator()(uintptr_t old, uintptr_t& result) const {
		result = value;
		return true;
	}

	inline ParamOverwriteCombiner(uintptr_t value): value(value) {}
};

template<class... Args>
class Scheduler<Args...>::IoJob::Launcher
{
	friend IoJob;

protected:
	typedef void (*Worker)(Launcher*, IoChannel*, Callback, Data* data);
	IoJob* job;

private:
	Worker worker;

protected:
	inline Launcher(IoJob* job, Worker worker): job(job), worker(worker) {}

    inline void jobSetup(IoChannel* channel, Callback callback, Data* data) {
        Registry<IoChannel>::check(channel);

        job->channel = channel;
        job->finished = callback;
        data->job = job;
        job->data = data;
    }

public:
	inline void launch(IoChannel* channel, Callback callback, Data* data) {
		worker(this, channel, callback, data);
	}

	inline IoJob* getJob() {
		return job;
	}
};

template<class... Args>
class Scheduler<Args...>::IoJob::NormalLauncher: public Launcher {
	static void worker(Launcher* launcher, IoChannel* channel, Callback callback, Data* data)
	{
		launcher->jobSetup(channel, callback, data);
		channel->add(launcher->job);
	}

public:
	NormalLauncher(IoJob* job): Launcher(job, &NormalLauncher::worker) {}
};

template<class... Args>
class Scheduler<Args...>::IoJob::TimeoutLauncher: public Launcher {
	uintptr_t time;

	static void worker(Launcher* launcher, IoChannel* channel, Callback callback, Data* data)
	{
		auto time = static_cast<TimeoutLauncher*>(launcher)->time;
        launcher->jobSetup(channel, callback, data);
        state.eventList.issue(launcher->job, ParamOverwriteCombiner(time)); // TODO assert true
	}

public:
	TimeoutLauncher(IoJob* job, uintptr_t time): Launcher(job, &TimeoutLauncher::worker), time(time) {}
};

}

#endif /* IOJOB_H_ */
