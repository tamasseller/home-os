/*
 * IoRequest.h
 *
 *  Created on: 2017.11.05.
 *      Author: tooma
 */

#ifndef IOREQUEST_H_
#define IOREQUEST_H_

#include "Scheduler.h"

template<class... Args>
class Scheduler<Args...>::IoRequestCommon:
		public Blocker,
		public WaitableBlocker<IoRequestCommon>,
		Registry<IoRequestCommon>::ObjectBase {

	friend Scheduler<Args...>;

	static constexpr uintptr_t timeoutReturnValue = 0;
	static constexpr uintptr_t blockedReturnValue = 1;

	Blockable* blocked = nullptr;

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

	virtual Blocker* getTakeable(Task* task) {
		// TODO explain in details, this is really crazy (re-virtualization of a de-virtualized call).
		return static_cast<Blocker*>(this)->getTakeable(task);
	}

protected:
	inline void resume() {
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
	void (* hijackedMethod)(typename IoChannel::Job*, typename IoChannel::Job::Result);

	static void activator(typename IoChannel::Job* job, typename IoChannel::Job::Result result) {
		IoRequest* self = static_cast<IoRequest*>(job);
		self->resume();
		self->hijackedMethod(job, result);
	}

	virtual Blocker* getTakeable(Task*) {
		return this->Job::channel ? nullptr : this;
	}

public:
	IoRequest() {
		hijackedMethod = this->Job::finished;
		this->Job::finished = &IoRequest::activator;
	}

	void init() {
		resetObject(this);
		Registry<IoRequestCommon>::registerObject(this);
	}

	~IoRequest() {
		Registry<IoRequestCommon>::unregisterObject(this);
	}
};


#endif /* IOREQUEST_H_ */
