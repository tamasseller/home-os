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

#ifndef IOREQUEST_H_
#define IOREQUEST_H_

#include "Scheduler.h"

namespace home {

template<class... Args>
class Scheduler<Args...>::IoRequestCommon:
		public Blocker,
		public WaitableBlocker<IoRequestCommon>,
		Registry<IoRequestCommon>::ObjectBase {

	friend Scheduler<Args...>;

	static constexpr uintptr_t timeoutReturnValue = 0;
	static constexpr uintptr_t blockedReturnValue = 1;

	Blockable* blocked = nullptr;
    typename IoJob::Callback hijackedMethod;
	typename IoJob::Result result = IoJob::Result::NotYet;

	virtual void priorityChanged(Blockable* b, typename Policy::Priority old) override {
		/*
		 * There is a single blocked entity at most, thus nothing
		 * needs to be done about the priority change.
		 */
	}

	virtual void canceled(Blockable* ba, Blocker* be) final override {
		blocked = nullptr;
	}

	virtual void timedOut(Blockable* ba) final override {
		Profile::injectReturnValue(blocked->getTask(), timeoutReturnValue);
		blocked = nullptr;
	}

	virtual inline void block(Blockable* b) override {
		assert(!blocked, ErrorStrings::ioRequestReuse);
		blocked = b;
	}

	virtual uintptr_t acquire(Task* task) {
		return 0;
	}

	/*
	 * Never called during normal operation. ( LCOV_EXCL_START )
	 */

	virtual bool release(uintptr_t arg) override final {
		assert(false, ErrorStrings::unreachableReached);
		return false;
	}

	/*
	 * The rest should be check for test coverage. ( LCOV_EXCL_STOP )
	 */

    virtual Blocker* getTakeable(Task*) override final {
        return (this->result == IoJob::Result::NotYet) ? nullptr : this;
    }

	virtual inline bool continuation(uintptr_t retval) override {
		// TODO explain in details, this is really crazy (re-virtualization of a de-virtualized call).
		return static_cast<Blocker*>(this)->continuation(retval);
	}

protected:
	class LauncherBase;
	class NormalLauncher;
	class ContinuationLauncher;
	class TimeoutLauncher;

public:
	inline void init() {
		Registry<IoRequestCommon>::registerObject(this);
	}

	inline typename IoJob::Result getResult() {
	    return result;
	}

	inline ~IoRequestCommon() {
		Registry<IoRequestCommon>::unregisterObject(this);
	}
};

template<class... Args>
class Scheduler<Args...>::IoRequestCommon::LauncherBase: IoJob::Launcher
{
	template<class> friend class IoRequest;
	friend class IoRequestCommon;

	IoJob* job;
	IoRequestCommon* self;
	typename IoJob::Callback activator;

	inline void commonOverwrite(IoChannel* channel, typename IoJob::Callback callback, typename IoJob::Data* data) {
		self->hijackedMethod = callback;
		job->finished = activator;
		data->job = job;
		job->data = data;
    }

	inline void forceOverwrite(IoChannel* channel, typename IoJob::Callback callback, typename IoJob::Data* data) {
		job->channel = channel;
		commonOverwrite(channel, callback, data);
	}

	inline bool takeOver(IoChannel* channel, typename IoJob::Callback callback, typename IoJob::Data* data) {
		if(!job->channel.compareAndSwap(nullptr, channel))
			return false;

		commonOverwrite(channel, callback, data);
		return true;
    }

	inline LauncherBase(IoRequestCommon* self, IoJob* job, typename IoJob::Callback activator, typename IoJob::Launcher::Worker worker):
		IoJob::Launcher(worker),
		job(job),
		self(self),
		activator(activator){}
};

template<class... Args>
class Scheduler<Args...>::IoRequestCommon::NormalLauncher: LauncherBase
{
	template<class> friend class IoRequest;

	static bool submit(typename IoJob::Launcher* launcher, IoChannel* channel, typename IoJob::Callback callback, typename IoJob::Data* data)
	{
		auto self = static_cast<NormalLauncher*>(launcher);

		if(!self->takeOver(channel, callback, data))
			return false;

		state.eventList.issue(self->job, typename IoJob::template OverwriteCombiner<IoJob::submitNoTimeoutValue>()); // TODO assert true
		return true;
	}

	NormalLauncher(IoRequestCommon* self, IoJob* job, typename IoJob::Callback activator):
		LauncherBase(self, job, activator, &NormalLauncher::submit) {}
};

template<class... Args>
class Scheduler<Args...>::IoRequestCommon::ContinuationLauncher: LauncherBase
{
	template<class> friend class IoRequest;

	static bool resubmit(typename IoJob::Launcher* launcher, IoChannel* channel, typename IoJob::Callback callback, typename IoJob::Data* data)
	{
		auto self = static_cast<ContinuationLauncher*>(launcher);

		Registry<IoChannel>::check(channel);
		self->forceOverwrite(channel, callback, data);

		state.eventList.issue(self->job, typename IoJob::template OverwriteCombiner<IoJob::submitNoTimeoutValue>()); // TODO assert true
		return true;
	}

	ContinuationLauncher(IoRequestCommon* self, IoJob* job, typename IoJob::Callback activator):
		LauncherBase(self, job, activator, &ContinuationLauncher::resubmit) {}
};

template<class... Args>
class Scheduler<Args...>::IoRequestCommon::TimeoutLauncher: LauncherBase
{
	template<class> friend class IoRequest;
	uintptr_t time;

	static bool submitTimeout(typename IoJob::Launcher* launcher, IoChannel* channel, typename IoJob::Callback callback, typename IoJob::Data* data)
	{
		auto self = static_cast<TimeoutLauncher*>(launcher);
		auto time = self->time;

		if(!time || time >= (uintptr_t)INTPTR_MAX)
			return false;

		if(!self->takeOver(channel, callback, data))
			return false;

		state.eventList.issue(self->job, typename IoJob::ParamOverwriteCombiner(time)); // TODO assert true
		return true;
	}

	TimeoutLauncher(IoRequestCommon* self, IoJob* job, typename IoJob::Callback activator, uintptr_t time):
		LauncherBase(self, job, activator, &TimeoutLauncher::submitTimeout), time(time) {}
};

template<class... Args>
template<class Job>
class Scheduler<Args...>::IoRequest:
		public Job,
		public IoRequestCommon
{
	typedef typename Scheduler<Args...>::IoJob IoJob;

	static bool activator(typename IoJob::Launcher* launcher, IoJob* job, typename IoJob::Result result) {
		IoRequest* self = static_cast<IoRequest*>(job);

		if (self->hijackedMethod)
			self->result = result;

		if (self->blocked) {
			self->blocked->wake(self);
			self->blocked = nullptr;
		}

		return true;
	}

	// TODO move to parent
	virtual bool continuation(uintptr_t retval) override final
	{
	    typename IoJob::Result result = this->result;
	    auto job = static_cast<IoJob*>(this);

		if(result == IoJob::Result::NotYet)// TODO fix WaitableSet, then use: if(!(bool)retval)
			return false;

	    this->result = IoJob::Result::NotYet;

	    typename IoRequestCommon::ContinuationLauncher launcher(this, this, &IoRequest::activator);

	    if(!this->hijackedMethod(&launcher, job, result)) {
	    	this->hijackedMethod = nullptr;

	    	if(this->isOccupied())
	    		this->cancel();		// Should complete immediately.

    		return false;
		}

		return this->isOccupied();
	}

public:
	inline void init() {
		resetObject(this);
		IoRequestCommon::init();
	}

	template<class Method, class... C>
	inline bool launch(Method method, C... c) {
		auto job = static_cast<IoJob*>(this);
		typename IoRequestCommon::NormalLauncher launcher(this, job, &IoRequest::activator);
		return method(&launcher, this, c...);
	}

	template<class Method, class... C>
	inline bool launchTimeout(Method method, uintptr_t time, C... c) {
		auto job = static_cast<IoJob*>(this);
		typename IoRequestCommon::TimeoutLauncher launcher(this, job, &IoRequest::activator, time);
		return method(&launcher, this, c...);
	}
};

}

#endif /* IOREQUEST_H_ */
