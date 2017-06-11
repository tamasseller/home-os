/*
 * Waitable.h
 *
 *  Created on: 2017.05.30.
 *      Author: tooma
 */

#ifndef WAITABLE_H_
#define WAITABLE_H_

#include "Scheduler.h"

#include "data/LinkedList.h"

template<class... Args>
class Scheduler<Args...>::Waitable: Blocker, Event {
	friend Scheduler<Args...>;
protected:

	// TODO this is unnecessary, remove and use the policy instead.
	class WakeSession {
		friend Waitable;
		Task* highestPrio = nullptr;
	};

	/**
	 * Wake up a task if there is any waiting.
	 *
	 * @return	True if a task was waken, false otherwise.
	 */
	bool wakeOne(WakeSession&);

private:
    /*
     * Implementation defined method to determine if
     * the synchronization object is blocking during
     * the acquire/lock/wait/pend call.
     *
	 * @return	Must return true if there the caller
	 * 			needs to be blocked, i.e. if the acquiring
	 * 			can not be successful at the moment, if
	 * 			returns true, the caller is blocked and
	 * 			put to the waiting list.
	 */
	virtual bool wouldBlock() = 0;

	/**
	 * Implementation defined acquire/lock/wait/pend method.
	 *
	 * @note	It is only called after establishing
	 * 			that it is going to be successfull by
	 * 			querying via the _wouldBlock_ method.
	 */
	virtual void acquire() = 0;

	/**
	 * Implementation defined release/unlock/notify/send
	 * method.
	 *
	 * @param	session The WakeSession that can be used
	 * 					to wake up waiters.
	 *
	 * @param	arg		The number of deferred release calls,
	 * 					the argument retrieved from the event
	 * 					list. See the description of EventList
	 * 					and AtomicList for details.
	 */
	virtual void release(WakeSession& session, uintptr_t arg) = 0;

	virtual void remove(Task* task) final;
	virtual void waken(Task* task, Waitable*) final;

	static bool comparePriority(const Blockable&, const Blockable&);
	pet::OrderedDoubleList<Blockable, &Waitable::comparePriority> waiters;

	static void doNotify(Event*, uintptr_t);
public:
	inline void init();
	inline void wait();
	inline bool wait(uintptr_t timeout);
	inline void notify();

	inline Waitable();
};

template<class... Args>
inline Scheduler<Args...>::Waitable::Waitable(): Event(doNotify) {}


template<class... Args>
inline void Scheduler<Args...>::Waitable::init() {
}

template<class... Args>
inline void Scheduler<Args...>::Waitable::wait() {
	using CallGate = typename Profile::CallGate;
	CallGate::sync(&Scheduler<Args...>::doWait, detypePtr(this));
}

template<class... Args>
inline bool Scheduler<Args...>::Waitable::wait(uintptr_t timeout) {
	using CallGate = typename Profile::CallGate;
	return CallGate::sync(&Scheduler<Args...>::doWaitTimeout, detypePtr(this), timeout);
}


template<class... Args>
inline void Scheduler<Args...>::Waitable::notify() {
	state.eventList.issue(this);
}

template<class... Args>
inline bool Scheduler<Args...>::Waitable::comparePriority(const Blockable& a, const Blockable& b) {
	return *a.getTask() < *b.getTask();
}

template<class... Args>
inline void Scheduler<Args...>::Waitable::remove(Task* task) {
	waiters.remove(task);
	task->injectReturnValue(false);
}

template<class... Args>
inline void Scheduler<Args...>::Waitable::waken(Task* task, Waitable*) {
	waiters.remove(task);
}

template<class... Args>
uintptr_t Scheduler<Args...>::doWait(uintptr_t waitablePtr)
{
	Waitable* waitable = entypePtr<Waitable>(waitablePtr);
	Task* currentTask = static_cast<Task*>(Profile::Task::getCurrent());

	if(waitable->wouldBlock()) {
		waitable->waiters.add(currentTask);
		currentTask->waitsFor = waitable;
		switchToNext<false>();
	} else
		waitable->acquire();

	return true;
}

template<class... Args>
uintptr_t Scheduler<Args...>::doWaitTimeout(uintptr_t waitablePtr, uintptr_t timeout)
{
	Waitable* waitable = entypePtr<Waitable>(waitablePtr);
	Task* currentTask = static_cast<Task*>(Profile::Task::getCurrent());

	if(!waitable->wouldBlock()) {
		waitable->acquire();
		return true;
	}

	if(!timeout)
		return false;

	waitable->waiters.add(currentTask);
	currentTask->waitsFor = waitable;
	state.sleepList.delay(currentTask, timeout);
	switchToNext<false>();

	/*
	 * False return value will be injected by
	 * the tick handler, if timed out.
	 */
	return true;
}

template<class... Args>
bool Scheduler<Args...>::Waitable::wakeOne(WakeSession& session) {
	if(Blockable* blockable = waiters.lowest()) {
		Task* waken = blockable->getTask();

		if(!session.highestPrio || *session.highestPrio < *waken)
			session.highestPrio = waken;

		if(waken->isSleeping())
			state.sleepList.remove(waken);

		waken->waitsFor->waken(waken, this);

		state.policy.addRunnable(waken);

		return true;
	}

	return false;
}

template<class... Args>
void Scheduler<Args...>::Waitable::doNotify(Event* event, uintptr_t arg)
{
	Waitable* waitable = static_cast<Waitable*>(event);
	Task* currentTask = static_cast<Task*>(Profile::Task::getCurrent());

	WakeSession session;
	waitable->release(session, arg);

	if(session.highestPrio && firstPreemptsSecond(session.highestPrio, currentTask))
		switchToNext<true>();
}

#endif /* WAITABLE_H_ */
