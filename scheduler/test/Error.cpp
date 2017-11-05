/*
 * Error.cpp
 *
 *  Created on: 2017.11.05.
 *      Author: tooma
 */

#include "common/CommonTestUtils.h"

using Os=OsRr;

TEST(Error1)
{
	struct Task: public TestTask<Task> {
		void run() {
			reinterpret_cast<Os::Mutex*>(0xf001d474)->lock();
		}
	} task;

//	task.start();
//	CommonTestUtils::start(Os::ErrorStrings::invalidSyscallArgument);
}
