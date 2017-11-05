/*
 * IoChannel.h
 *
 *  Created on: 2017.11.05.
 *      Author: tooma
 */

#ifndef IOCHANNEL_H_
#define IOCHANNEL_H_

#include "Scheduler.h"

template<class... Args>
class Scheduler<Args...>::IoChannel {
	friend Scheduler<Args...>;

	virtual void enableProcess() = 0;
	virtual void disableProcess() = 0;

public:
	class Job: Timeout {
		friend Scheduler<Args...>;
		friend pet::DoubleList<Job>;

		Job *next, *prev;
		IoChannel * volatile channel = nullptr;
		void (* const finished)(Job*, bool);

		static void timedOut(Sleeper* sleeper) {
			Job* job = static_cast<Job*>(sleeper);

			if(IoChannel *channel = job->channel) {
				channel->disableProcess();

				bool ret = channel->requests.remove(job);

				if(ret)
					job->channel = nullptr;

				channel->enableProcess();

				if(ret)
					job->finished(job, false);
			}
		}

		static inline void nop(Job* job, bool succesful) {}
	public:

		inline Job(void (* const finished)(Job*, bool) = &Job::nop): Timeout(&Job::timedOut), finished(finished) {}
	};

private:

	pet::DoubleList<Job> requests;

protected:

	Job* currentJob() {
		return requests.front();
	}

	void jobDone() {
		Job* job = requests.popFront();
		job->finished(job, true);

		if(!requests.front())
			disableProcess();

		job->channel = nullptr;

		if(job->isSleeping())
			job->cancel();
	}

public:
	void submit(Job* job)
	{
		disableProcess();

		requests.addBack(job);
		job->channel = this;

		enableProcess();
	}

	void submitTimeout(Job* job, uintptr_t time)
	{
		submit(job);

		job->start(time);
	}

	void cancel(Job* job)
	{
		job->cancel();

		disableProcess();

		if(requests.remove(job))
			job->channel = nullptr;

		if(!requests.front())
			disableProcess();
	}
};

#endif /* IOCHANNEL_H_ */
