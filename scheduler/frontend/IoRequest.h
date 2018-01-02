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

    virtual Blocker* getTakeable(Task* task) override {
    	// TODO explain in details, this is really crazy (re-virtualization of a de-virtualized call).
    	return static_cast<Blocker*>(this)->getTakeable(task);
    }

	virtual inline bool continuation(uintptr_t retval) override {
		// TODO explain in details, this is really crazy (re-virtualization of a de-virtualized call).
		return static_cast<Blocker*>(this)->continuation(retval);
	}

protected:
	class RequestLauncher;
	class NormalRequestLauncher;
	class TimeoutRequestLauncher;

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
class Scheduler<Args...>::IoRequestCommon::RequestLauncher: public IoJob::Launcher
{
	friend class IoRequestCommon;

	IoRequestCommon* self;
	typename IoJob::Callback activator;

    inline void jobSetup(IoChannel* channel, typename IoJob::Callback callback, typename IoJob::Data* data) {
        IoJob::Launcher::jobSetup(channel, activator, data);
		self->hijackedMethod = callback;
	}

	inline RequestLauncher(IoRequestCommon* self, IoJob* job, typename IoJob::Callback activator, typename IoJob::Launcher::Worker worker):
		IoJob::Launcher(job, worker), self(self), activator(activator){}
};

template<class... Args>
class Scheduler<Args...>::IoRequestCommon::NormalRequestLauncher: public RequestLauncher {
    static void worker(typename IoJob::Launcher* launcher, IoChannel* channel, typename IoJob::Callback callback, typename IoJob::Data* data)
    {
        auto reqLauncher= static_cast<RequestLauncher*>(launcher);
        reqLauncher->jobSetup(channel, callback, data);
        channel->add(reqLauncher->job);
    }

public:
    NormalRequestLauncher(IoRequestCommon* self, IoJob* job, typename IoJob::Callback activator):
        RequestLauncher(self, job, activator, &NormalRequestLauncher::worker) {}
};

template<class... Args>
class Scheduler<Args...>::IoRequestCommon::TimeoutRequestLauncher: public RequestLauncher {
    uintptr_t time;

    static void worker(typename IoJob::Launcher* launcher, IoChannel* channel, typename IoJob::Callback callback, typename IoJob::Data* data)
    {
        auto reqLauncher= static_cast<RequestLauncher*>(launcher);
        auto time = static_cast<TimeoutRequestLauncher*>(launcher)->time;

        reqLauncher->jobSetup(channel, callback, data);
        state.eventList.issue(reqLauncher->job, typename IoJob::ParamOverwriteCombiner(time));
    }

public:
    TimeoutRequestLauncher(IoRequestCommon* self, IoJob* job, typename IoJob::Callback activator, uintptr_t time):
        RequestLauncher(self, job, activator, &TimeoutRequestLauncher::worker), time(time) {}
};

template<class... Args>
template<class Job, bool (Job::*onBlocking)()>
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

	virtual bool continuation(uintptr_t retval) override final
	{
		// TODO explain in details, this is really crazy (re-virtualization of a de-virtualized call).
	    typename IoJob::Result result = this->result;
	    auto job = static_cast<IoJob*>(this);

		if(result == IoJob::Result::NotYet)// TODO fix WaitableSet, then use: if(!(bool)retval)
			return false;

	    this->result = IoJob::Result::NotYet;

	    typename IoRequestCommon::NormalRequestLauncher launcher(this, this, &IoRequest::activator);

	    if(!this->hijackedMethod(&launcher, job, result)) {
	    	this->hijackedMethod = nullptr;

	    	if(this->isOccupied())
	    		this->cancel();		// Should complete immediately.

    		return false;
		}

		return this->isOccupied();
	}

    virtual Blocker* getTakeable(Task* task) override final {
    	// TODO explain in details, this is really crazy (re-virtualization of a de-virtualized call).

    	if(this->result != IoJob::Result::NotYet)
    		return this;

    	if(onBlocking && !(this->*onBlocking)())
    		return this;

        return nullptr;
    }

public:
	inline void init() {
		resetObject(this);
		IoRequestCommon::init();
	}

	template<class Method, class... C>
	inline bool launch(Method method, C... c) {
		auto job = static_cast<IoJob*>(this);
		typename IoRequestCommon::NormalRequestLauncher launcher(this, job, &IoRequest::activator);
        return this->launchWithLauncher(&launcher, method, c...);
	}

	template<class Method, class... C>
	inline bool launchTimeout(Method method, uintptr_t time, C... c) {
        if(!time || time >= (uintptr_t)INTPTR_MAX)
            return false;

		auto job = static_cast<IoJob*>(this);
		typename IoRequestCommon::TimeoutRequestLauncher launcher(this, job, &IoRequest::activator, time);
		return this->launchWithLauncher(&launcher, method, c...);
	}
};

}

#endif /* IOREQUEST_H_ */
