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

#ifndef DUMMYPROCESS_H_
#define DUMMYPROCESS_H_

#include "CommonTestUtils.h"

template<class Os>
class DummyProcess: public Os::template IoChannelBase<DummyProcess<Os>> {
public:
	volatile unsigned int counter;

	static DummyProcess instance;

	void init() {
		this->jobs.clear();
		counter = 0;
		this->DummyProcess::IoChannelBase::init();
	}

private:
	friend class DummyProcess::IoChannelBase;

	static inline void processWorkerIsr() {
		if(instance.counter) {
			typename Os::IoJob* job = instance.jobs.front();
			int* param = reinterpret_cast<int*>(job->param);

			auto count = instance.counter--;

			if(param)
				*param = count;

			instance.jobDone(job);
		}
	}

	bool addJob(typename Os::IoJob* job) {
		return this->jobs.addBack(job);
	}

	bool removeJob(typename Os::IoJob* job) {
		return this->jobs.remove(job);
	}

	void enableProcess() {
		CommonTestUtils::registerIrq(&DummyProcess::processWorkerIsr);
	}

	void disableProcess() {
		CommonTestUtils::registerIrq(nullptr);
	}
};

template<class Os>
DummyProcess<Os> DummyProcess<Os>::instance;

template<class Os>
class SemaphoreProcess: public Os::template IoChannelBase<SemaphoreProcess<Os>>
{
	friend class SemaphoreProcess::IoChannelBase;

	int count;

	bool addJob(typename Os::IoJob* job)
	{
		if(static_cast<int>(job->param) < 0 && this->jobs.front())
			return this->jobs.addBack(job);

		while(job) {
			int param = static_cast<int>(job->param);

			const int newVal = count + param;

			if(newVal < 0) {
				return this->jobs.addBack(job);
			}

			count = newVal;
			this->jobDone(job);

			job = this->jobs.front();
		}

		return true;
	}

	bool removeJob(typename Os::IoJob* job) {
		this->jobs.remove(job);
		return true;
	}

	void enableProcess() {}
	void disableProcess() {}

public:

	void init() {
		this->jobs.clear();
		count = 0;
		this->SemaphoreProcess::IoChannelBase::init();
	}

	static SemaphoreProcess instance;
};

template<class Os>
SemaphoreProcess<Os> SemaphoreProcess<Os>::instance;

template<class Os>
struct DummyProcessJobsBase {
	using Process = DummyProcess<Os>;
	using SemProcess = SemaphoreProcess<Os>;
	constexpr static Process &process = Process::instance;
	constexpr static SemProcess &semProcess = SemProcess::instance;

	class Job: public Os::IoJob {
		static bool writeResult(typename Os::IoJob* item, typename Os::IoJob::Result result, void (*hook)(typename Os::IoJob*)) {
			Job* job = static_cast<Job*>(item);
			job->success = (int)result;
			return false;
		}

		template <class> friend class Os::IoChannelBase;
		template <class> friend class Os::IoRequest;
		inline void prepare() {
			success = -1;
			this->Os::IoJob::prepare(&Job::writeResult, reinterpret_cast<uintptr_t>(&count));
		}

	public:
		volatile int success;
		volatile int count;

		inline Job(): success(-1), count(-1) {}
	};

	template<size_t n>
	struct MultiJob: Os::IoJob {
		template<int> struct Selector {};

		static bool writeResult(typename Os::IoJob* item, typename Os::IoJob::Result result, void (*hook)(typename Os::IoJob*)) {
			MultiJob* job = static_cast<MultiJob*>(item);
			job->success[--job->idx] = (int)result;

			if(job->idx) {
				Process::instance.resubmit(hook, item, &MultiJob::writeResult);
				return true;
			}

			return false;
		}

		template <class> friend class Os::IoChannelBase;
		template <class> friend class Os::IoRequest;

		inline void prepare() {
			for(auto &x: success)
				x = -1;

			this->Os::IoJob::prepare(&MultiJob::writeResult);
		}
	public:
		volatile int idx = n;
		volatile int success[n];

	};

	class SemJob: Os::IoJob {
		static bool callback(typename Os::IoJob* item, typename Os::IoJob::Result result, void (*hook)(typename Os::IoJob*)) {
			static_cast<SemJob*>(item)->done = true;
			return false;
		}

		template <class> friend class Os::IoChannelBase;
		template <class> friend class Os::IoRequest;

		inline void prepare(int n) {
			done = false;
			this->Os::IoJob::prepare(&SemJob::callback, n);
		}

	public:
		volatile bool done;

	};

	struct CompositeJob: Os::IoJob {

		static bool step2(typename Os::IoJob* item, typename Os::IoJob::Result result, void (*hook)(typename Os::IoJob*))
		{
		    auto self = static_cast<CompositeJob*>(item);

		    if(result == Os::IoJob::Result::TimedOut)
		        self->timedOut = true;
            else
                self->stage = 2;

			return false;
		}

		static bool step1(typename Os::IoJob* item, typename Os::IoJob::Result result, void (*hook)(typename Os::IoJob*)) {
		    auto self = static_cast<CompositeJob*>(item);

		    self->stage = 1;

			if(self->timeout)
				Process::instance.resubmitTimeout(hook, item, self->timeout, &CompositeJob::step2);
			else
				Process::instance.resubmit(hook, item, &CompositeJob::step2);

			return true;
		}

		friend class Os::IoChannel;
		template <class> friend class Os::IoRequest;

		inline void prepare(int n, uintptr_t timeout = 0) {
			stage = 0;
			timedOut = false;
			this->timeout = timeout;
			this->Os::IoJob::prepare(&CompositeJob::step1, n);
		}

	public:
		volatile bool timedOut;
		uintptr_t timeout;
		volatile int stage;
	};

	typedef typename Os::template IoRequest<Job> Req;
	Job jobs[5];
	Req reqs[3];

	bool postJobs()
	{
		bool ret = true;

		for(unsigned int i=0; i<sizeof(jobs)/sizeof(jobs[0]); i++) {
			if(!DummyProcess<Os>::instance.submitTimeout(jobs + i, 100))
				ret = false;
		}

		return ret;
	}

	bool postJobsNoTimeout()
	{
		bool ret = true;

		for(unsigned int i=0; i<sizeof(jobs)/sizeof(jobs[0]); i++) {
			if(!DummyProcess<Os>::instance.submit(jobs + i))
				ret = false;
		}

		return ret;
	}

	bool postReqsNoTimeout() {
		for(unsigned int i=0; i<sizeof(reqs)/sizeof(reqs[0]); i++) {
			reqs[i].init();
			if(!DummyProcess<Os>::instance.submit(reqs + i))
				return false;
		}

		return true;
	}

	int checkJobs()
	{
		int ret;

		while(true) {
			bool done = true;
			ret = 0;

			for(unsigned int i=0; i<sizeof(jobs)/sizeof(jobs[0]); i++) {
				if(jobs[i].success == -1)
					done = false;
				else if(jobs[i].success == (int)Os::IoJob::Result::Done)
					ret++;
			}

			if(done)
				break;
		}

		return ret;
	}
};


#endif /* DUMMYPROCESS_H_ */
