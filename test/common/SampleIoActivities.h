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
		friend typename Os::IoJob;

		static bool writeResult(typename Os::IoJob::Launcher* launcher, typename Os::IoJob* item, typename Os::IoJob::Result result) {
			Job* self = static_cast<Job*>(item);
			self->success = (int)result;
			return false;
		}

		static bool start(typename Os::IoJob::Launcher* launcher, typename Os::IoJob* item) {
			Job* self = static_cast<Job*>(item);
			self->success = -1;
			self->param = &self->count;
			return launcher->launch(&DummyProcess<Os>::instance, &Job::writeResult, self);
		}

	public:
		static constexpr auto entry = &Job::start;
		volatile int success;
		volatile int count;


		inline Job(): success(-1), count(-1) {}

	};

	template<size_t n>
	struct MultiJob: Os::IoJob, public DummyProcess<Os>::Data {
		template<int> struct Selector {};

		static bool writeResult(typename Os::IoJob::Launcher* launcher, typename Os::IoJob* item, typename Os::IoJob::Result result) {
			MultiJob* self = static_cast<MultiJob*>(item);

			self->success[--self->idx] = (int)result;

			if(self->idx) {
				launcher->launch(&Process::instance, &MultiJob::writeResult, self);
				return true;
			}

			return false;
		}

		static bool start(typename Os::IoJob::Launcher* launcher, typename Os::IoJob* item) {
			MultiJob* self = static_cast<MultiJob*>(item);

			for(auto &x: self->success)
				x = -1;

			self->param = 0;
			return launcher->launch(&process, &MultiJob::writeResult, self);
		}

	public:
		static constexpr auto entry = &MultiJob::start;
		volatile int idx = n;
		volatile int success[n];
	};

	class SemJob: public Os::IoJob, public SemaphoreProcess<Os>::Data  {
		static bool callback(typename Os::IoJob::Launcher* launcher, typename Os::IoJob* item, typename Os::IoJob::Result result) {
			static_cast<SemJob*>(item)->done = true;
			return false;
		}

		friend class Os::IoChannel;
		template <class> friend class Os::IoRequest;

		static bool start(typename Os::IoJob::Launcher* launcher, typename Os::IoJob* item, int n) {
			auto self = static_cast<SemJob*>(item);
			self->done = false;
			self->param = n;
			return launcher->launch(&SemaphoreProcess<Os>::instance, &SemJob::callback, self);
		}
	public:
		static constexpr auto entry = &SemJob::start;

		volatile bool done;
	};

	class CompositeJob: public Os::IoJob {

		union {
			typename DummyProcess<Os>::Data dummyData;
			typename SemaphoreProcess<Os>::Data semData;
		};

		static bool step2(typename Os::IoJob::Launcher* launcher, typename Os::IoJob* item, typename Os::IoJob::Result result)
		{
		    auto self = static_cast<CompositeJob*>(item);

		    if(result == Os::IoJob::Result::TimedOut)
		        self->timedOut = true;
            else
                self->stage = 2;

			return false;
		}

		static bool step1(typename Os::IoJob::Launcher* launcher, typename Os::IoJob* item, typename Os::IoJob::Result result) {
		    auto self = static_cast<CompositeJob*>(item);

		    self->stage = 1;
		    self->dummyData.param = 0;
		    launcher->launch(&Process::instance, &CompositeJob::step2, &self->dummyData);

			return true;
		}

		static bool start(typename Os::IoJob::Launcher* launcher, typename Os::IoJob* item, int n)
		{
			auto self = static_cast<CompositeJob*>(item);

			self->stage = 0;
			self->timedOut = false;
			self->semData.param = n;

			return launcher->launch(&SemaphoreProcess<Os>::instance, &CompositeJob::step1, &self->semData);
		}

	public:
		static constexpr auto entry = &CompositeJob::start;

		volatile bool timedOut;
		volatile int stage;
	};

	typedef typename Os::template IoRequest<Job> Req;
	Req reqs[3];
	Job jobs[5];

	bool postJobs()
	{
		bool ret = true;

		for(unsigned int i=0; i<sizeof(jobs)/sizeof(jobs[0]); i++) {
			if(!jobs[i].launchTimeout(Job::entry, 100))
				ret = false;
		}

		return ret;
	}

	bool postJobsNoTimeout()
	{
		bool ret = true;

		for(unsigned int i=0; i<sizeof(jobs)/sizeof(jobs[0]); i++) {
			if(!jobs[i].launch(Job::entry))
				ret = false;
		}

		return ret;
	}

	bool postReqsNoTimeout() {
		for(unsigned int i=0; i<sizeof(reqs)/sizeof(reqs[0]); i++) {
			reqs[i].init();
			if(!reqs[i].launch(Req::entry))
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
