/*
 * IoChannelTimeout.cpp
 *
 *  Created on: 2017.11.05.
 *      Author: tooma
 */

#include "common/CommonTestUtils.h"
#include "common/DummyProcess.h"

using Os=OsRr;
using Base = DummyProcessJobsBase<Os>;
static auto &process = DummyProcess<Os>::instance;

TEST(IoChannel) {
	struct Task: Base, public TestTask<Task> {
		bool error = false;
		void run() {
			process.counter = 0;

			if(!postJobsNoTimeout()) {
				error = true;
				return;
			}

			Os::sleep(50);
			process.counter = 5;

			if(checkJobs() != 5) {
				error = true;
				return;
			}
		}
	} task;

	process.init();
	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(IoChannelTimeout) {
	struct Task: Base, public TestTask<Task> {
		bool error = false;
		void run() {
			process.counter = 0;
			for(int i=sizeof(jobs)/sizeof(jobs[0]); i>=0; i--) {

				if(!postJobs()) {
					error = true;
					return;
				}

				process.counter = i;

				if(checkJobs() != i) {
					error = true;
					return;
				}
			}
		}
	} task;

	process.init();
	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(IoChannelTimeoutPostpone) {
	struct Task: Base, public TestTask<Task> {
		bool error = false;
		void run() {
			process.counter = 0;

			if(!postJobs()) {
				error = true;
				return;
			}

			Os::sleep(50);

			for(unsigned int i=0; i<sizeof(jobs)/sizeof(jobs[0]); i++)
				if(jobs[i].success != -1)
					error = true;

			if(postJobs()) {
				error = true;
				return;
			}

			process.counter = 5;

			if(checkJobs() != 5)
				error = true;
		}
	} task;

	process.init();
	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(IoChannelTimeoutDoTimout) {
	struct Task: Base, public TestTask<Task> {
		bool error = false;
		void run() {
			process.counter = 0;

			if(!postJobs()) {
				error = true;
				return;
			}

			Os::sleep(110);

			for(unsigned int i=0; i<sizeof(jobs)/sizeof(jobs[0]); i++)
				if(jobs[i].success != (int)Os::IoChannel::Job::Result::TimedOut)
					error = true;
		}
	} task;

	process.init();
	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}


TEST(IoChannelTimeoutCancel) {
	struct Task: Base, public TestTask<Task> {
		bool error = false;
		void run() {
			process.counter = 0;

			if(!postJobs()) {
				error = true;
				return;
			}

			Os::sleep(40);

			for(unsigned int i=0; i<sizeof(jobs)/sizeof(jobs[0]); i++)
				process.cancel(jobs + i);

			Os::sleep(10);

			process.counter = 5;

			Os::sleep(50);

			for(unsigned int i=0; i<sizeof(jobs)/sizeof(jobs[0]); i++)
				if(jobs[i].success != (int)Os::IoChannel::Job::Result::Canceled)
					error = true;
		}
	} task;

	process.init();
	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}
