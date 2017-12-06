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
		Ok, NotPresent, Raced, Denied
	};

	virtual RemoveResult remove(IoJob* job, typename IoJob::Result result) = 0;
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
template<class Child, class Base>
class Scheduler<Args...>::IoChannelBase: public Base {
	friend class Scheduler<Args...>;
	using RemoveResult = typename IoChannelCommon::RemoveResult;
	using Data = typename IoJob::Data;

	inline void enableProcess() {}
	inline void disableProcess() {}
	inline bool addItem(Data*);

	inline bool removeItem(Data*);	// TODO check if still needed (or can be done by
									// the user and removeCanceled is enough here)

	inline bool removeCanceled(Data* data) {return static_cast<Child*>(this)->removeItem(data);}
	inline bool hasJob();

	virtual RemoveResult remove(IoJob* job, typename IoJob::Result result) override final
	{
		auto self = static_cast<Child*>(this);

		self->disableProcess();

		Profile::memoryFence();

		if(job->channel != this) {
			self->enableProcess();
			return RemoveResult::Raced;
		}

		if(result == IoJob::Result::Done) {
			if(!self->removeItem(job->data))
				return RemoveResult::NotPresent;
		} else {
			if(!self->removeCanceled(job->data))
				return RemoveResult::Denied;
		}

		job->channel = nullptr;

		if(self->hasJob())
			self->enableProcess();

		return RemoveResult::Ok;
	}

	virtual bool add(IoJob* job) override final
	{
		auto self = static_cast<Child*>(this);
		self->disableProcess();
		bool ret = self->addItem(job->data);
		self->enableProcess();
		return ret;
	}

protected:
	void jobDone(Data* data) {
		auto self = static_cast<Child*>(this);
		assert(data->job, ErrorStrings::ioRequestState);

		/*
		 * TODO describe relevance in locking mechanism of removeSynchronized.
		 */

		data->job->channel = nullptr;

		Profile::memoryFence();

		bool ok = self->removeItem(data);
		assert(ok, ErrorStrings::ioRequestState);

		if(!self->hasJob())
			self->disableProcess();

		state.eventList.issue(data->job, typename IoJob::template OverwriteCombiner<IoJob::doneValue>());
	}
public:
	template<class... C>
	inline IoChannelBase(C... c): Base(c...) {}

	inline IoChannelBase() = default;
};

}

#endif /* IOCHANNEL_H_ */
