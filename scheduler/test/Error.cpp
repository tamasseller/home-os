/*
 * Error.cpp
 *
 *  Created on: 2017.11.05.
 *      Author: tooma
 */

#include "common/CommonTestUtils.h"

using Os=OsRr;

TEST_GROUP(Error) {};

TEST(Error, InvalidSyscallArgumentSync)
{
	struct Task: public TestTask<Task> {
		void run() {
			reinterpret_cast<Os::Mutex*>(0xbadf00d0)->lock();
		}
	} task;

	task.start();
	CommonTestUtils::start(Os::ErrorStrings::invalidSyscallArgument);
}

TEST(Error, InvalidSyscallArgumentAsync)
{
	struct Task: public TestTask<Task> {
		static void frobnicator() {
			CommonTestUtils::registerIrq(nullptr);
			reinterpret_cast<Os::CountingSemaphore*>(0xbadf00d0)->notifyFromInterrupt();
		}

		void run() {
			CommonTestUtils::registerIrq(&frobnicator);
			while(1);
		}
	} task;

	task.start();
	CommonTestUtils::start(Os::ErrorStrings::invalidSyscallArgument);
}

TEST(Error, MutexNonOwnerUnlock)
{
	struct Task: public TestTask<Task> {
		void run() {
			Os::Mutex m;
			m.init();
			m.unlock();
			while(1);
		}
	} task;

	task.start();
	CommonTestUtils::start(Os::ErrorStrings::mutexNonOwnerUnlock);
}

TEST(Error, TaskDelayTooBig)
{
	struct Task: public TestTask<Task> {
		void run() {
			Os::sleep((uintptr_t)-1);
			while(1);
		}
	} task;

	task.start();
	CommonTestUtils::start(Os::ErrorStrings::taskDelayTooBig);
}
