/*
 * DummyProcess.h
 *
 *  Created on: 2017.11.08.
 *      Author: tooma
 */

#ifndef DUMMYPROCESS_H_
#define DUMMYPROCESS_H_

#include "CommonTestUtils.h"

template<class Os>
class DummyProcess: public Os::IoChannel {
public:
	inline virtual ~DummyProcess() {}

	struct Job: Os::IoChannel::Job {
		Job *next, *prev;
		int success;

		static void writeResult(typename Os::IoChannel::Job* item, typename Os::IoChannel::Job::Result result) {
			Job* job = static_cast<Job*>(item);
			job->success = (int)result;
		}

	public:
		inline Job(): Os::IoChannel::Job(&Job::writeResult), success(-1) {}
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

	pet::DoubleList<Job> requests;

	virtual bool addJob(typename Os::IoChannel::Job* job) {
		return requests.addBack(static_cast<Job*>(job));
	}

	virtual bool removeJob(typename Os::IoChannel::Job* job) {
		return requests.remove(static_cast<Job*>(job));
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
