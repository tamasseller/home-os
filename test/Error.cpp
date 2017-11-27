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

#include "common/CommonTestUtils.h"

using Os=OsRr;

TEST_GROUP(Error) {};

TEST(Error, InvalidSyscallArgumentSync)
{
	struct Task: public TestTask<Task> {
		bool run() {
			reinterpret_cast<Os::Mutex*>(0xbadf00d0)->lock();
			return ok;
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

		bool run() {
			CommonTestUtils::registerIrq(&frobnicator);
			while(1);
			return ok;
		}
	} task;

	task.start();
	CommonTestUtils::start(Os::ErrorStrings::invalidSyscallArgument);
}

TEST(Error, MutexNonOwnerUnlock)
{
	struct Task: public TestTask<Task> {
		bool run() {
			Os::Mutex m;
			m.init();
			m.unlock();
			while(1);
			return ok;
		}
	} task;

	task.start();
	CommonTestUtils::start(Os::ErrorStrings::mutexNonOwnerUnlock);
}

TEST(Error, TaskDelayTooBig)
{
	struct Task: public TestTask<Task> {
		bool run() {
			Os::sleep((uintptr_t)-1);
			while(1);
			return ok;
		}
	} task;

	task.start();
	CommonTestUtils::start(Os::ErrorStrings::taskDelayTooBig);
}
