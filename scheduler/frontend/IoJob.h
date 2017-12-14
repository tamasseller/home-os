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

	class LauncherBase;
	class NormalLauncher;
	class ContinuationLauncher;
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
				ContinuationLauncher launcher(this);
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
				ContinuationLauncher launcher(self);
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

public:
	inline IoJob(): Sleeper(&IoJob::timedOut), Event(&IoJob::handleRequest) {}

	inline bool isOccupied() {
		return this->channel != nullptr;
	}

	template<class Method, class... C>
	inline bool launch(Method method, C... c) {
		NormalLauncher launcher(this);
		return method(&launcher, this, c...);
	}

	template<class Method, class... C>
	inline bool launchTimeout(Method method, uintptr_t time, C... c) {
		TimeoutLauncher launcher(this, time);
		return method(&launcher, this, c...);
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

public:
	typedef void (*Initializer)(IoJob*);

protected:
	typedef bool (*Worker)(Launcher*, IoChannel*, Callback, Data* data, Initializer init);

private:
	Worker worker;

protected:
	inline Launcher(Worker worker): worker(worker) {}

public:
	inline bool launch(IoChannel* channel, Callback callback, Data* data, Initializer init = nullptr) {
		return worker(this, channel, callback, data, init);
	}
};

template<class... Args>
class Scheduler<Args...>::IoJob::LauncherBase: Launcher
{
	friend IoJob;

	IoJob* job;

	inline void commonOverwrite(IoChannel* channel, Callback callback, Data* data) {
		job->finished = callback;
		data->job = job;
		job->data = data;
    }

	inline void forceOverwrite(IoChannel* channel, Callback callback, Data* data) {
		job->channel = channel;
		commonOverwrite(channel, callback, data);
	}

	inline bool takeOver(IoChannel* channel, Callback callback, Data* data) {
		if(!job->channel.compareAndSwap(nullptr, channel))
			return false;

		commonOverwrite(channel, callback, data);
		return true;
    }

	inline LauncherBase(IoJob* job, typename Launcher::Worker worker): Launcher(worker), job(job) {}
};

template<class... Args>
class Scheduler<Args...>::IoJob::NormalLauncher: LauncherBase
{
	friend IoJob;

	static bool submit(
			Launcher* launcher,
			IoChannel* channel,
			Callback callback,
			Data* data,
			typename Launcher::Initializer init)
	{
		auto self = static_cast<NormalLauncher*>(launcher);

		if(!self->takeOver(channel, callback, data))
			return false;

		if(init)
			init(self->job);

		state.eventList.issue(self->job, OverwriteCombiner<IoJob::submitNoTimeoutValue>()); // TODO assert true
		return true;
	}

	NormalLauncher(IoJob* job): LauncherBase(job, &NormalLauncher::submit) {}
};

template<class... Args>
class Scheduler<Args...>::IoJob::ContinuationLauncher: LauncherBase
{
	friend IoJob;

	static bool resubmit(
			Launcher* launcher,
			IoChannel* channel,
			Callback callback,
			Data* data,
			typename Launcher::Initializer init)
	{
		auto self = static_cast<ContinuationLauncher*>(launcher);

		Registry<IoChannel>::check(channel);
		self->forceOverwrite(channel, callback, data);

		if(init)
			init(self->job);

		return channel->add(self->job);
	}

	ContinuationLauncher(IoJob* job): LauncherBase(job, &ContinuationLauncher::resubmit) {}
};

template<class... Args>
class Scheduler<Args...>::IoJob::TimeoutLauncher: LauncherBase
{
	friend IoJob;
	uintptr_t time;

	static bool submitTimeout(
			Launcher* launcher,
			IoChannel* channel,
			Callback callback,
			Data* data,
			typename Launcher::Initializer init)
	{
		auto self = static_cast<TimeoutLauncher*>(launcher);
		auto time = self->time;

		if(!time || time >= (uintptr_t)INTPTR_MAX)
			return false;

		if(!self->takeOver(channel, callback, data))
			return false;

		if(init)
			init(self->job);

		state.eventList.issue(self->job, ParamOverwriteCombiner(time)); // TODO assert true
		return true;
	}

	TimeoutLauncher(IoJob* job, uintptr_t time): LauncherBase(job, &TimeoutLauncher::submitTimeout), time(time) {}
};

}

#endif /* IOJOB_H_ */
