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
class Scheduler<Args...>::Waitable: Waker, Event {
	friend Scheduler<Args...>;
protected:
	/**
	 * Represents a series of tasks waken during a single
	 * run of the release method of the implementation.
	 */
	class WakeSession {
		friend Waitable;

		/// The highest priority task waken during the session.
		Task* highestPrio = nullptr;
	};

	/**
	 * Wake up a task if there is any waiting.
	 *
	 * @return	True if a task was waken, false otherwise.
	 */
	bool wakeOne(WakeSession&);

private:
	/**
	 * Implementation defined acquire/lock/wait/pend method.
	 *
	 * @return	Must return true if there the caller needs
	 * 			**NOT** to be blocked, i.e. if the acquiring
	 * 			was successful, if returns false, the caller
	 * 			is blocked and put to the waiting list.
	 */
	virtual bool acquire() = 0;

	/**
	 * Implementation defined release/unlock/notify/send method.
	 *
	 * @param	session The WakeSession that can be used to wake
	 * 					up waiters.
	 *
	 * @param	arg		The argument retrieved from EventList.
	 * 					See the description of EventList and
	 * 					AtomicList for details.
	 */
	virtual void release(WakeSession& session, uintptr_t arg) = 0;

	virtual void remove(Task* task) final;
	virtual void waken(Task* task) final;

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
	Profile::CallGate::sync(&Scheduler<Args...>::doWait, detypePtr(this));
}

template<class... Args>
inline bool Scheduler<Args...>::Waitable::wait(uintptr_t timeout) {
	return Profile::CallGate::sync(&Scheduler<Args...>::doWaitTimeout, detypePtr(this), timeout);
}


template<class... Args>
inline void Scheduler<Args...>::Waitable::notify() {
	postEvent(this);
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
inline void Scheduler<Args...>::Waitable::waken(Task* task) {
	waiters.remove(task);
}

template<class... Args>
uintptr_t Scheduler<Args...>::doWait(uintptr_t waitablePtr)
{
	Waitable* waitable = entypePtr<Waitable>(waitablePtr);
	Task* currentTask = static_cast<Task*>(Profile::Task::getCurrent());

	if(!waitable->acquire()) {
		waitable->waiters.add(currentTask);
		currentTask->waitsFor = waitable;
		switchToNext<false>();
	}
}

template<class... Args>
uintptr_t Scheduler<Args...>::doWaitTimeout(uintptr_t waitablePtr, uintptr_t timeout)
{
	Waitable* waitable = entypePtr<Waitable>(waitablePtr);
	Task* currentTask = static_cast<Task*>(Profile::Task::getCurrent());

	if(waitable->acquire())
		return true;

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

		waken->waitsFor->waken(waken);

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

	if(session.highestPrio && *currentTask < *session.highestPrio)
		switchToNext<true>();
}

#endif /* WAITABLE_H_ */
