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
class DummyProcess: public Os::IoChannel {
public:
	inline virtual ~DummyProcess() {}

	volatile unsigned int counter;

	static DummyProcess instance;

	void init() {
		this->jobs.clear();
		counter = 0;
		this->Os::IoChannel::init();
	}

private:
	static inline void processWorkerIsr() {
		if(instance.counter) {
			typename Os::IoChannel::Job* job = instance.jobs.front();
			int* param = reinterpret_cast<int*>(job->param);

			auto count = instance.counter--;

			if(param)
				*param = count;

			instance.jobDone(job);
		}
	}

	virtual bool addJob(typename Os::IoChannel::Job* job) {
		return this->jobs.addBack(job);
	}

	virtual bool removeJob(typename Os::IoChannel::Job* job) {
		return this->jobs.remove(job);
	}

	virtual void enableProcess() {
		CommonTestUtils::registerIrq(&DummyProcess::processWorkerIsr);
	}

	virtual void disableProcess() {
		CommonTestUtils::registerIrq(nullptr);
	}
};

template<class Os>
DummyProcess<Os> DummyProcess<Os>::instance;

template<class Os>
class SemaphoreProcess: public Os::IoChannel
{
	int count;

	virtual bool addJob(typename Os::IoChannel::Job* job)
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

	virtual bool removeJob(typename Os::IoChannel::Job* job) {
		this->jobs.remove(job);
		return true;
	}

	virtual void enableProcess() {}
	virtual void disableProcess() {}

public:

	void init() {
		this->jobs.clear();
		count = 0;
		this->Os::IoChannel::init();
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

	struct Job: Os::IoChannel::Job {
		volatile int success;
		volatile int count;

		static bool writeResult(typename Os::IoChannel::Job* item, typename Os::IoChannel::Job::Result result, const typename Os::IoChannel::Job::Reactivator &) {
			Job* job = static_cast<Job*>(item);
			job->success = (int)result;
			return false;
		}

		inline void prepare() {
			success = -1;
			this->Os::IoChannel::Job::prepare(&Job::writeResult, reinterpret_cast<uintptr_t>(&count));
		}

	public:
		inline Job(): success(-1), count(-1) {}
	};

	template<size_t n>
	struct MultiJob: Os::IoChannel::Job {
		volatile int idx = n;
		volatile int success[n];

		static bool writeResult(typename Os::IoChannel::Job* item, typename Os::IoChannel::Job::Result result, const typename Os::IoChannel::Job::Reactivator &react) {
			MultiJob* job = static_cast<MultiJob*>(item);
			job->success[--job->idx] = (int)result;

			if(job->idx) {
				job->Os::IoChannel::Job::prepare(&MultiJob::writeResult);
				react.reactivate(item, &Process::instance);
				return true;
			}

			return false;
		}

	public:
		inline void prepare() {
			for(auto &x: success)
				x = -1;

			this->Os::IoChannel::Job::prepare(&MultiJob::writeResult);
		}
	};

	struct SemJob: Os::IoChannel::Job {
		volatile bool done;

		static bool callback(typename Os::IoChannel::Job* item, typename Os::IoChannel::Job::Result result, const typename Os::IoChannel::Job::Reactivator &react) {
			static_cast<SemJob*>(item)->done = true;
			return false;
		}

		inline void prepare(int n) {
			done = false;
			this->Os::IoChannel::Job::prepare(&SemJob::callback, n);
		}
	};

	struct CompositeJob: Os::IoChannel::Job {
		volatile int stage;

		static bool step2(typename Os::IoChannel::Job* item, typename Os::IoChannel::Job::Result result, const typename Os::IoChannel::Job::Reactivator &react) {
			static_cast<CompositeJob*>(item)->stage = 2;
			return false;
		}


		static bool step1(typename Os::IoChannel::Job* item, typename Os::IoChannel::Job::Result result, const typename Os::IoChannel::Job::Reactivator &react) {
			static_cast<CompositeJob*>(item)->stage = 1;
			static_cast<CompositeJob*>(item)->Os::IoChannel::Job::prepare(&CompositeJob::step2);
			react.reactivate(item, &Process::instance);
			return true;
		}

	public:
		inline void prepare(int n) {
			stage = 0;
			this->Os::IoChannel::Job::prepare(&CompositeJob::step1, n);
		}
	};

	typedef typename Os::template IoRequest<Job> Req;
	Job jobs[5];
	Req reqs[3];

	bool postJobs()
	{
		bool ret = true;

		for(unsigned int i=0; i<sizeof(jobs)/sizeof(jobs[0]); i++) {
			jobs[i].prepare();
			if(!DummyProcess<Os>::instance.submitTimeout(jobs + i, 100))
				ret = false;
		}

		return ret;
	}

	bool postJobsNoTimeout()
	{
		bool ret = true;

		for(unsigned int i=0; i<sizeof(jobs)/sizeof(jobs[0]); i++) {
			jobs[i].prepare();
			if(!DummyProcess<Os>::instance.submit(jobs + i))
				ret = false;
		}

		return ret;
	}

	bool postReqsNoTimeout() {
		for(unsigned int i=0; i<sizeof(reqs)/sizeof(reqs[0]); i++) {
			reqs[i].init();
			reqs[i].prepare();
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
				else if(jobs[i].success == (int)Os::IoChannel::Job::Result::Done)
					ret++;
			}

			if(done)
				break;
		}

		return ret;
	}
};


#endif /* DUMMYPROCESS_H_ */
