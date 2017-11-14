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
	typename IoChannel::Job::Result result = IoChannel::Job::Result::NotYet;

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
	 * This two method are never called during normal operation, so
	 * LCOV_EXCL_START is placed here to exclude them from coverage analysis
	 */

	virtual bool release(uintptr_t arg) override final {
		assert(false, ErrorStrings::unreachableReached);
		return false;
	}

	/*
	 * From here on, the rest should be check for test coverage, so
	 * LCOV_EXCL_STOP is placed here.
	 */

    virtual Blocker* getTakeable(Task*) override final {
        return (this->result == IoChannel::Job::Result::NotYet) ? nullptr : this;
    }

	virtual bool continuation(uintptr_t retval) override {
		// TODO explain in details, this is really crazy (re-virtualization of a de-virtualized call).
		return static_cast<Blocker*>(this)->continuation(retval);
	}

protected:
	inline void resume(typename IoChannel::Job::Result result) {
		this->result = result;
		if(blocked) {
			blocked->wake(this);
			blocked = nullptr;
		}
	}
};

template<class... Args>
template<class Job>
class Scheduler<Args...>::IoRequest:
		public Job,
		public IoRequestCommon
{
    typedef bool (*Method)(typename IoChannel::Job*, typename IoChannel::Job::Result, const typename IoChannel::Job::Reactivator &);
    Method hijackedMethod;

    class HijackReactivator: public IoChannel::Job::Reactivator {
        virtual bool reactivate(typename IoChannel::Job* job, IoChannel* channel) const override final {
            return channel->submit(job);
        }

        virtual bool reactivateTimeout(typename IoChannel::Job* job, IoChannel* channel, uintptr_t timeout) const override final {
            return channel->submitTimeout(job, timeout);
        }
    };

	static bool activator(typename IoChannel::Job* job, typename IoChannel::Job::Result result, const typename IoChannel::Job::Reactivator &) {
		IoRequest* self = static_cast<IoRequest*>(job);
		self->resume(result);
		return false;
	}

	virtual bool continuation(uintptr_t) override final {
	    typename IoChannel::Job::Result result = this->result;
	    this->result = IoChannel::Job::Result::NotYet;
		return hijackedMethod(this, result, HijackReactivator());
	}

public:
	inline IoRequest(typename IoChannel::Job* job) {
		hijackedMethod = job->finished;
		job->finished = &IoRequest::activator;
	}

	inline IoRequest(): IoRequest(this) {}

	inline void init() {
		resetObject(this);
		Registry<IoRequestCommon>::registerObject(this);
	}

	inline ~IoRequest() {
		Registry<IoRequestCommon>::unregisterObject(this);
	}
};

}

#endif /* IOREQUEST_H_ */
