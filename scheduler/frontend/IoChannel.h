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

#ifndef IOCHANNEL_H_
#define IOCHANNEL_H_

#include "Scheduler.h"

namespace home {

template<class... Args>
class Scheduler<Args...>::IoChannel: Registry<IoChannel>::ObjectBase { // TODO add registration
	friend Scheduler<Args...>;

private:
	virtual void enableProcess() = 0;
	virtual void disableProcess() = 0;
	virtual bool addJob(IoJob*) = 0;
	virtual bool removeJob(IoJob*) = 0;

	template<uintptr_t value>
	struct OverwriteCombiner {
		inline bool operator()(uintptr_t old, uintptr_t& result) const {
			result = value;
			return true;
		}
	};

    inline bool takeJob(IoJob* job);

	inline bool hasJob() {
		return jobs.front() != nullptr;
	}

	inline bool submitPrepared(IoJob* job);
	inline bool submitTimeoutPrepared(IoJob* job, uintptr_t time);

protected:
	pet::DoubleList<IoJob> jobs;

	void jobDone(IoJob* job);

public:
	template<class ActualJob, class... C>
	inline bool submit(ActualJob* job, C... c);

	template<class ActualJob, class... C>
	inline bool submitTimeout(ActualJob* job, uintptr_t time, C... c);

	void cancel(IoJob* job);

	void init();
	~IoChannel();
};

template<class... Args>
inline bool Scheduler<Args...>::IoChannel::takeJob(IoJob* job) {
	return job->channel.compareAndSwap(nullptr, this);
}

template<class... Args>
inline void Scheduler<Args...>::IoChannel::jobDone(IoJob* job) {
	/*
	 * TODO describe relevance in locking mechanism of removeSynchronized.
	 */
	job->channel = nullptr;

	Profile::memoryFence();

	removeJob(job);

	if(!hasJob())
		disableProcess();

	state.eventList.issue(job, OverwriteCombiner<IoJob::doneValue>());
}

template<class... Args>
inline void Scheduler<Args...>::IoChannel::init() {
	Registry<IoChannel>::registerObject(this);
}

template<class... Args>
template<class ActualJob, class... C>
inline bool Scheduler<Args...>::IoChannel::submit(ActualJob* job, C... c)
{
	if(!takeJob(job))
		return false;

	job->prepare(c...);

	state.eventList.issue(static_cast<IoJob*>(job), OverwriteCombiner<IoJob::submitNoTimeoutValue>());
	return true;
}

template<class... Args>
template<class ActualJob, class... C>
inline bool Scheduler<Args...>::IoChannel::submitTimeout(ActualJob* job, uintptr_t time, C... c)
{
	if(!time || time >= (uintptr_t)INTPTR_MAX)
		return false;

	if(!takeJob(job))
		return false;

	job->prepare(c...);

	state.eventList.issue(static_cast<IoJob*>(job), [time](uintptr_t, uintptr_t& result) {
		result = time;
		return true;
	});

	return true;
}

template<class... Args>
inline bool Scheduler<Args...>::IoChannel::submitPrepared(IoJob* job)
{
	if(!takeJob(job))
		return false;

	state.eventList.issue(static_cast<IoJob*>(job), OverwriteCombiner<IoJob::submitNoTimeoutValue>());
	return true;
}

template<class... Args>
inline bool Scheduler<Args...>::IoChannel::submitTimeoutPrepared(IoJob* job, uintptr_t time)
{
	if(!time || time >= (uintptr_t)INTPTR_MAX)
		return false;

	if(!takeJob(job))
		return false;

	state.eventList.issue(static_cast<IoJob*>(job), [time](uintptr_t, uintptr_t& result) {
		result = time;
		return true;
	});

	return true;
}

template<class... Args>
inline void Scheduler<Args...>::IoChannel::cancel(IoJob* job)
{
	state.eventList.issue(job, OverwriteCombiner<IoJob::cancelValue>());
}

template<class... Args>
inline Scheduler<Args...>::IoChannel::~IoChannel() {
	/*
	 * Io channels must be deleted very carefully, because there
	 * can be jobs referencing it that can trigger execution in
	 * various asynchronous contexts.
	 */

	assert(!state.isRunning, ErrorStrings::ioChannelDelete);
}

}

#endif /* IOCHANNEL_H_ */
