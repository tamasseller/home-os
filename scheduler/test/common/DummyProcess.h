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

	struct JobBase: Os::IoChannel::Job {
		JobBase *next, *prev;

		inline JobBase(bool (*f)(typename Os::IoChannel::Job*, typename Os::IoChannel::Job::Result, const typename Os::IoChannel::Job::Reactivator &)):
				Os::IoChannel::Job(f) {}
	};

	struct Job: JobBase {
		int success;

		static bool writeResult(typename Os::IoChannel::Job* item, typename Os::IoChannel::Job::Result result, const typename Os::IoChannel::Job::Reactivator &) {
			Job* job = static_cast<Job*>(item);
			job->success = (int)result;
			return false;
		}

	public:
		inline Job(): JobBase(&Job::writeResult), success(-1) {}
	};

	template<size_t n>
	struct MultiJob: JobBase {
		volatile int idx = n;
		int success[n];

		static bool writeResult(typename Os::IoChannel::Job* item, typename Os::IoChannel::Job::Result result, const typename Os::IoChannel::Job::Reactivator &) {
			MultiJob* job = static_cast<MultiJob*>(item);
			job->success[--job->idx] = (int)result;

			if(job->idx) {
				instance.submit(item);
				return true;
			}

			return false;
		}

	public:
		inline MultiJob(): JobBase(&MultiJob::writeResult) {
			for(auto &x: success)
				x = -1;
		}
	};


	volatile unsigned int counter;

	static DummyProcess instance;

private:
	static inline void processWorkerIsr() {
		if(instance.counter) {
			instance.counter--;
			instance.jobDone(instance.requests.front());
		}
	}

	pet::DoubleList<JobBase> requests;

	virtual bool addJob(typename Os::IoChannel::Job* job) {
		return requests.addBack(static_cast<JobBase*>(job));
	}

	virtual bool removeJob(typename Os::IoChannel::Job* job) {
		return requests.remove(static_cast<JobBase*>(job));
	}

	virtual bool hasJob() {
		return requests.front() != nullptr;
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
struct DummyProcessJobsBase {
	typedef typename DummyProcess<Os>::Job Job;
	typedef typename Os::template IoRequest<Job> Req;
	Job jobs[5];
	Req reqs[3];

	bool postJobs()
	{
		bool ret = true;

		for(unsigned int i=0; i<sizeof(jobs)/sizeof(jobs[0]); i++) {
			jobs[i].success = -1;

			if(!DummyProcess<Os>::instance.submitTimeout(jobs + i, 100))
				ret = false;
		}

		return ret;
	}

	bool postJobsNoTimeout()
	{
		bool ret = true;

		for(unsigned int i=0; i<sizeof(jobs)/sizeof(jobs[0]); i++) {
			jobs[i].success = -1;

			if(!DummyProcess<Os>::instance.submit(jobs + i))
				ret = false;
		}

		return ret;
	}

	bool postReqsNoTimeout() {
		for(unsigned int i=0; i<sizeof(reqs)/sizeof(reqs[0]); i++) {
			reqs[i].init();
			reqs[i].success = -1;
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
