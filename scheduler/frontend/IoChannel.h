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
class Scheduler<Args...>::IoChannel {
	friend Scheduler<Args...>;

	enum class RemoveResult {
		Ok, NotPresent, Raced
	};

	virtual RemoveResult remove(IoJob* job) = 0;
	virtual bool add(IoJob* job) = 0;
};

template<class... Args>
class Scheduler<Args...>::IoChannelCommon: Registry<IoChannelCommon>::ObjectBase, public IoChannel
{
	friend class Scheduler<Args...>;

public:
	inline void init() {
		Registry<IoChannelCommon>::registerObject(this);
	}

	~IoChannelCommon() {
		/*
		 * Io channels must be deleted very carefully, because there
		 * can be jobs referencing it that can trigger execution in
		 * various asynchronous contexts.
		 */

		assert(!state.isRunning, ErrorStrings::ioChannelDelete);
	}
};


template<class... Args>
template<class Child>
class Scheduler<Args...>::IoChannelBase: public IoChannelCommon {
	friend class Scheduler<Args...>;
	using RemoveResult = typename IoChannelCommon::RemoveResult;

	inline void enableProcess() {}
	inline void disableProcess() {}
	inline bool addJob(IoJob*);
	inline bool removeJob(IoJob*);

	virtual RemoveResult remove(IoJob* job) override final
	{
		auto self = static_cast<Child*>(this);

		self->disableProcess();

		Profile::memoryFence();

		if(job->channel != this) {
			self->enableProcess();
			return RemoveResult::Raced;
		}

		bool ok = self->removeJob(job);
		job->channel = nullptr;

		if(self->hasJob())
			self->enableProcess();

		return ok ? RemoveResult::Ok : RemoveResult::NotPresent;
	}

	virtual bool add(IoJob* job) override final
	{
		auto self = static_cast<Child*>(this);
		self->disableProcess();
		bool ret = self->addJob(job);
		self->enableProcess();
		return ret;
	}

protected:
	pet::DoubleList<IoJob> jobs;

	void jobDone(IoJob* job) {
		auto self = static_cast<Child*>(this);
		/*
		 * TODO describe relevance in locking mechanism of removeSynchronized.
		 */
		job->channel = nullptr;

		Profile::memoryFence();

		self->removeJob(job);

		if(!self->hasJob())
			self->disableProcess();

		state.eventList.issue(job, typename IoJob::template OverwriteCombiner<IoJob::doneValue>());
	}

	inline bool hasJob() {
		return jobs.front() != nullptr;
	}
};

}

#endif /* IOCHANNEL_H_ */
