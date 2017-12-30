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

/**
 * Interface for handlers of asynchronous events.
 *
 * Instances of implementations of this interface implement asynchronous
 * input/output functions in conjunction _IoJob_ objects, which use these
 * implementations via this interface.
 *
 * The implementations of these methods are responsible for proper locking in
 * case the process driven via the channel is asynchronous (ie. interrupt driven).
 */
template<class... Args>
class Scheduler<Args...>::IoChannel: Registry<IoChannel>::ObjectBase {
	friend Scheduler<Args...>;

	/**
	 * The result of an remove operation.
	 */

protected:
	enum class RemoveResult {
		Ok,				///< The job is not present in the channel queue (anymore).
		Denied,			///< The channel rejected the removal (may be in progress).
		Raced 			///< The job was found not to be owned by this channel.
	};

private:

	/**
	 * Try to remove the specified job.
	 *
	 * If successful the channel may not initiate a callback on this job anymore.
	 * This operation may fail for multiple reasons:
	 *
	 *  - The job is not present in the channels queue or
	 *  - The job can not be removed due to some implementation defined reason (for
	 *    example if it is being processed by an non-interruptible mechanism) or
	 *  - The job is not owned by this channel, this status can only be a result of
	 *    a race condition, because the channel is reached through the reference of
	 *    the job. So if this kicks-in the reference must have been changed between
	 *    the fetching of the pointer to the channel and the method invocation.
	 */
	virtual RemoveResult remove(IoJob* job, typename IoJob::Result result) = 0;

	/**
	 * Register a job for processing
	 *
	 * The job can be stored in an internal queue of the channel or it can be
	 * processed immediately depending on the specific implementation.
	 */
	virtual bool add(IoJob* job) = 0;

public:
	/**
	 * Trivial constructor, to enable flexible placement.
	 *
	 * The channel **needs to be initialized** once the scheduler is up and running.
	 */
	IoChannel() = default;

	/**
	 * Runtime initialization.
	 *
	 * Adds the object to the registry (if enabled).
	 */
	inline void init() {
		Registry<IoChannel>::registerObject(this);
	}

	/**
	 * Check safe disposal.
	 *
	 * An _IoChannel_ must be deleted very carefully, because there can be
	 * jobs in-flight, referencing it in various asynchronous contexts.
	 */
	~IoChannel() {
		assert(!state.isRunning, ErrorStrings::ioChannelDelete);
	}

	/**
	 * Deleted copy constructors and assignment operators.
	 *
	 * IoChannels should be stable and uniquely referenceable entities
	 * throughout the whole runtime operation, because the references
	 * to them are used in scenarios where race conditions pose tight
	 * restrictions on the runtime sanity checks that can be performed
	 * about them.
	 */
	IoChannel(const IoChannel&) = delete;
	IoChannel(IoChannel&&) = delete;
	void operator =(const IoChannel&) = delete;
	void operator =(IoChannel&&) = delete;
};

/**
 * Implementation template of a regular IoChannel.
 *
 * This implementation expects the process to be interrupt driven,
 * and routes completion notifications through the asynchronous
 * event dispatch mechanism.
 *
 * User implementations are supposed to use this in CRTP fashion.
 */
template<class... Args>
template<class Child, class Base>
class Scheduler<Args...>::IoChannelBase: public Base {
	friend class Scheduler<Args...>;
	using RemoveResult = typename IoChannel::RemoveResult;
	using Data = typename IoJob::Data;

	/**
	 * Enable the asynchronous process.
	 *
	 * Enable the interrupt associated with the channel.
	 */
	inline void enableProcess() {}

	/**
	 * Disable the asynchronous process.
	 *
	 * Disable the interrupt associated with the channel.
	 */
	inline void disableProcess() {}

	/**
	 * Start processing an item.
	 *
	 * Start processing or queue the job if can not be started immediately,
	 * once the job is done the process shall call the _jobDone_ method to
	 * signal this (possibly from an interrupt handler).
	 */
	inline bool addItem(Data*);

	/**
	 * TODO
	 */
	inline bool removeCanceled(Data* data);

	/**
	 * Query the presence of waiting jobs.
	 *
	 * This method is used by the template to find out whether the process needs to be
	 * re-enabled after an erase operation or disabled after a job completion event.
	 */
	inline bool hasJob();

	/**
	 * Implementation of the _IoChannel_ interface.
	 *
	 * The process specific functionality is accessed based on the CRTP
	 * accessible methods provided by the user and described above.
	 */
	virtual RemoveResult remove(IoJob* job, typename IoJob::Result result) override final
	{
		auto self = static_cast<Child*>(this);

		self->disableProcess();

		Profile::memoryFence();

		if(job->channel != this) {
			self->enableProcess();
			return RemoveResult::Raced;
		}

		bool ret = self->removeCanceled(job->data);
		if(ret)
			job->channel = nullptr;

		if(self->hasJob())
			self->enableProcess();

		return ret ? RemoveResult::Ok : RemoveResult::Denied;
	}

	/**
	 * Implementation of the _IoChannel_ interface.
	 *
	 * The process specific functionality is accessed based on the CRTP
	 * accessible methods provided by the user and described above.
	 */
	virtual bool add(IoJob* job) override final
	{
		auto self = static_cast<Child*>(this);

		self->disableProcess();

		bool ret = self->addItem(job->data);

		if(self->hasJob())
			self->enableProcess();

		return ret;
	}

protected:

	/**
	 * Notify the listener of the job of completion.
	 *
	 * This method is called from the user provided implementation when the
	 * processing of a job has completed. It may be called from an interrupt
	 * service routine or the _addItem_ method as described above.
	 */
	void jobDone(Data* data) {
		auto self = static_cast<Child*>(this);
		assert(data->job, ErrorStrings::ioRequestState);

		if(!self->hasJob())
			self->disableProcess();

		state.eventList.issue(data->job, typename IoJob::template OverwriteCombiner<IoJob::doneValue>());
	}

public:
	template<class... C>
	inline IoChannelBase(C... c): Base(c...) {}

	inline IoChannelBase() = default;
};

/**
 * Implementation template of a synchronous IoChannel.
 *
 * This implementation template can be used when the completion of
 * the jobs is done in a non-interrupt driven way, although locking
 * needs to be implemented (for example via the asynchronous event
 * dispatching).
 *
 * User implementations are supposed to use this in CRTP fashion.
 */
template<class... Args>
template<class Child, class Base>
class Scheduler<Args...>::SynchronousIoChannelBase: public Base {
	friend class Scheduler<Args...>;
	using RemoveResult = typename IoChannel::RemoveResult;
	using Data = typename IoJob::Data;

	/**
	 * Start processing an item.
	 *
	 * Start processing or queue the job if can not be started immediately, once
	 * the job is done the process shall call the _jobDone_ method to signal this.
	 *
	 * @NOTE It is called from a synchronized context.
	 */
	inline bool addItem(Data*);

	/**
	 * TODO
	 */
	inline bool removeCanceled(Data* data);

	/**
	 * Implementation of the _IoChannel_ interface.
	 *
	 * The process specific functionality is accessed based on the CRTP
	 * accessible methods provided by the user and described above.
	 */
	virtual RemoveResult remove(IoJob* job, typename IoJob::Result result) override final
	{
		auto self = static_cast<Child*>(this);

		bool ret = self->removeCanceled(job->data);

		if(ret)
			job->channel = nullptr;

		return ret ? RemoveResult::Ok : RemoveResult::Denied;
	}

	/**
	 * Implementation of the _IoChannel_ interface.
	 *
	 * The process specific functionality is accessed based on the CRTP
	 * accessible methods provided by the user and described above.
	 */
	virtual bool add(IoJob* job) override final
	{
		auto self = static_cast<Child*>(this);
		return self->addItem(job->data);
	}

protected:

	/**
	 * Notify the listener of the job of completion.
	 *
	 * This method is called from the user provided implementation when the
	 * processing of a job has completed.
	 *
	 * @NOTE
	 * 		It must only be called from a synchronized context (including an invocation
	 * 		of the _addItem_ method).
	 */
	void jobDone(Data* data) {
		auto self = static_cast<Child*>(this);
		assert(data->job, ErrorStrings::ioRequestState);
		(data->job->getCallback())(data->job, IoJob::doneValue);
	}

public:
	template<class... C>
	inline SynchronousIoChannelBase(C... c): Base(c...) {}

	inline SynchronousIoChannelBase() = default;
};

}

#endif /* IOCHANNEL_H_ */
