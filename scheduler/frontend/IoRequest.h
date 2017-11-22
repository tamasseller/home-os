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
        return (this->result == IoJob::Result::NotYet) ? nullptr : this;
    }

	virtual bool continuation(uintptr_t retval) override {
		// TODO explain in details, this is really crazy (re-virtualization of a de-virtualized call).
		return static_cast<Blocker*>(this)->continuation(retval);
	}

protected:
	inline void resume(typename IoJob::Result result) {
		this->result = result;
		if(blocked) {
			blocked->wake(this);
			blocked = nullptr;
		}
	}

public:
	inline typename IoJob::Result getResult() {
	    return result;
	}
};

template<class... Args>
template<class Job>
class Scheduler<Args...>::IoRequest:
		public Job,
		public IoRequestCommon
{
	typedef typename Scheduler<Args...>::IoJob IoJob;

	// TODO move to parent
    typedef bool (*Method)(IoJob*, typename IoJob::Result, void (*hook)(IoJob*));
    Method hijackedMethod;

    static inline void hijack(IoJob* job) {
    	IoRequest* self = static_cast<IoRequest*>(static_cast<Job*>(job));
    	self->hijackedMethod = self->IoJob::finished;
		self->IoJob::finished = &IoRequest::activator;
		self->result = IoJob::Result::NotYet;
    }

	static bool activator(IoJob* job, typename IoJob::Result result, void (*hook)(IoJob*)) {
		IoRequest* self = static_cast<IoRequest*>(job);
		self->resume(result);
		return false;
	}

	// TODO move to parent
	virtual bool continuation(uintptr_t) override final
	{
		if(this->result == IoJob::Result::NotYet)
			return false;

	    typename IoJob::Result result = this->result;
	    this->result = IoJob::Result::NotYet;
		return hijackedMethod(this, result, &IoRequest::hijack);
	}

public:
	inline void init() {
		resetObject(this);
		Registry<IoRequestCommon>::registerObject(this);
	}

	template<class... C>
	inline bool start(C... c) {
		return this->Job::start(c..., &IoRequest::hijack);
	}

	template<class... C>
	inline bool startTimeout(C... c) {
		return this->Job::startTimeout(c..., &IoRequest::hijack);
	}

	inline ~IoRequest() {
		Registry<IoRequestCommon>::unregisterObject(this);
	}
};

}

#endif /* IOREQUEST_H_ */
