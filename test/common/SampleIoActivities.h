/*
 * SampleIoActivities.h
 *
 *  Created on: 2017.11.21.
 *      Author: tooma
 */

#ifndef SAMPLEIOACTIVITIES_H_
#define SAMPLEIOACTIVITIES_H_

#include "DummyProcess.h"
#include "SemaphoreProcess.h"

template<class Os>
struct DummyProcessJobsBase {
	using Process = DummyProcess<Os>;
	using SemProcess = SemaphoreProcess<Os>;
	constexpr static Process &process = Process::instance;
	constexpr static SemProcess &semProcess = SemProcess::instance;

	/* TODO check if visibility can be decreased */
	class Job: public Os::IoJob, public DummyProcess<Os>::Data {
		static bool writeResult(typename Os::IoJob* item, typename Os::IoJob::Result result, void (*hook)(typename Os::IoJob*)) {
			Job* job = static_cast<Job*>(item);
			job->success = (int)result;
			return false;
		}

	public:
		volatile int success;
		volatile int count;

		inline Job(): success(-1), count(-1) {}

		bool start(typename Os::IoJob::Hook hook = nullptr) {
			success = -1;
			this->param = &count;
			return this->submit(hook, &DummyProcess<Os>::instance, &Job::writeResult, this);
		}

		bool startTimeout(uintptr_t timeout, typename Os::IoJob::Hook hook = nullptr) {
			success = -1;
			this->param = 0;
			return this->submitTimeout(hook, &DummyProcess<Os>::instance, timeout, &Job::writeResult, this);
		}
	};

	template<size_t n>
	struct MultiJob: Os::IoJob, public DummyProcess<Os>::Data {
		template<int> struct Selector {};

		static bool writeResult(typename Os::IoJob* item, typename Os::IoJob::Result result, void (*hook)(typename Os::IoJob*)) {
			MultiJob* job = static_cast<MultiJob*>(item);
			job->success[--job->idx] = (int)result;

			if(job->idx) {
				job->submit(hook, &Process::instance, &MultiJob::writeResult, job);
				return true;
			}

			return false;
		}

	public:
		volatile int idx = n;
		volatile int success[n];

		inline bool start(typename Os::IoJob::Hook hook = nullptr) {
			for(auto &x: success)
				x = -1;

			this->param = 0;
			return this->submit(hook, &process, &MultiJob::writeResult, this);
		}
	};

	class SemJob: Os::IoJob, public SemaphoreProcess<Os>::Data  {
		static bool callback(typename Os::IoJob* item, typename Os::IoJob::Result result, void (*hook)(typename Os::IoJob*)) {
			static_cast<SemJob*>(item)->done = true;
			return false;
		}

		friend class Os::IoChannel;
		template <class> friend class Os::IoRequest;

	public:
		volatile bool done;

		bool start(int n, typename Os::IoJob::Hook hook = nullptr) {
			done = false;
			this->param = n;
			return this->submit(hook, &SemaphoreProcess<Os>::instance, &SemJob::callback, this);
		}
	};

	struct CompositeJob: Os::IoJob {

		union {
			typename DummyProcess<Os>::Data dummyData;
			typename SemaphoreProcess<Os>::Data semData;
		};

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
		    self->dummyData.param = 0;

			if(self->timeout)
				self->submitTimeout(hook, &Process::instance, self->timeout, &CompositeJob::step2, &self->dummyData);
			else
				self->submit(hook, &Process::instance, &CompositeJob::step2, &self->dummyData);

			return true;
		}

	public:

		bool start(int n, uintptr_t timeout, typename Os::IoJob::Hook hook = nullptr) {
			stage = 0;
			timedOut = false;
			this->timeout = timeout;
			this->semData.param = n;

			return this->submit(hook, &SemaphoreProcess<Os>::instance, &CompositeJob::step1, &this->semData);
		}

		volatile bool timedOut;
		uintptr_t timeout;
		volatile int stage;
	};

	typedef typename Os::template IoRequest<Job> Req;
	Req reqs[3];
	Job jobs[5];

	bool postJobs()
	{
		bool ret = true;

		for(unsigned int i=0; i<sizeof(jobs)/sizeof(jobs[0]); i++) {
			if(!jobs[i].startTimeout(100))
				ret = false;
		}

		return ret;
	}

	bool postJobsNoTimeout()
	{
		bool ret = true;

		for(unsigned int i=0; i<sizeof(jobs)/sizeof(jobs[0]); i++) {
			if(!jobs[i].start())
				ret = false;
		}

		return ret;
	}

	bool postReqsNoTimeout() {
		for(unsigned int i=0; i<sizeof(reqs)/sizeof(reqs[0]); i++) {
			reqs[i].init();
			if(!reqs[i].start())
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

#endif /* SAMPLEIOACTIVITIES_H_ */
