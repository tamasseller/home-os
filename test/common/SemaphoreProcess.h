/*
 * SemaphoreProcess.h
 *
 *  Created on: 2017.11.21.
 *      Author: tooma
 */

#ifndef SEMAPHOREPROCESS_H_
#define SEMAPHOREPROCESS_H_

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




#endif /* SEMAPHOREPROCESS_H_ */
