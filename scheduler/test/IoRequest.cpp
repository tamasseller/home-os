/*
 * IoRequest.cpp
 *
 *  Created on: 2017.11.08.
 *      Author: tooma
 */

#include "common/CommonTestUtils.h"
#include "common/DummyProcess.h"

using Os=OsRr;
using Process = DummyProcess<Os>;
using Base = DummyProcessJobsBase<Os>;
static auto &process = Process::instance;

TEST(IoRequestAlreadyDone) {
	struct Task: public TestTask<Task>, Base  {
		bool error = false;
		void run() {
			process.counter = 0;

			if(!postReqsNoTimeout()) {
				error = true;
				return;
			}

			process.counter = 3;

			Os::sleep(10);

			if(process.counter) {
				error = true;
				return;
			}

			for(unsigned int i=0; i<sizeof(reqs)/sizeof(reqs[0]); i++) {
				reqs[i].wait();

				if(reqs[i].success != (int)Os::IoChannel::Job::Result::Done) {
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

TEST(IoRequestWaitTimeout) {
	struct Task: public TestTask<Task>, Base  {
		bool error = false;
		void run() {
			process.counter = 0;

			if(!postReqsNoTimeout()) {
				error = true;
				return;
			}

			for(unsigned int i=0; i<sizeof(reqs)/sizeof(reqs[0]); i++) {
				bool ok = reqs[i].wait(10);

				if(ok) {
					error = true;
					return;
				}
			}

			process.counter = 3;

			for(unsigned int i=0; i<sizeof(reqs)/sizeof(reqs[0]); i++) {
				bool ok = reqs[i].wait(10);

				if(!ok || reqs[i].success != (int)Os::IoChannel::Job::Result::Done) {
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

TEST(IoRequestWaitNoTimeout) {
	struct Task: public TestTask<Task>, Base  {
		bool error = false;
		void run() {
			process.counter = 0;

			if(!postReqsNoTimeout()) {
				error = true;
				return;
			}

			process.counter = 3;

			for(unsigned int i=0; i<sizeof(reqs)/sizeof(reqs[0]); i++) {
				reqs[i].wait();

				if(reqs[i].success != (int)Os::IoChannel::Job::Result::Done) {
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

TEST(IoRequestSelect) {
	struct Task: public TestTask<Task>, Base {
		bool error = false;
		void run() {
			process.counter = 0;

			if(!postReqsNoTimeout()) {
				error = true;
				return;
			}

			process.counter = 3;

			auto* s1 = Os::select(&reqs[0], &reqs[1], &reqs[2]);

			if(s1 != &reqs[0] || reqs[0].success != (int)Os::IoChannel::Job::Result::Done) {
				error = true;
				return;
			}

			auto* s2 = Os::select(&reqs[1], &reqs[2]);

			if(s2 != &reqs[1] || reqs[1].success != (int)Os::IoChannel::Job::Result::Done) {
				error = true;
				return;
			}

			auto* s3 = Os::select(&reqs[2]);

			if(s3 != &reqs[2] || reqs[2].success != (int)Os::IoChannel::Job::Result::Done) {
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

TEST(IoRequestIoTimeoutSelect) {
	struct Task: public TestTask<Task>, Base {
		bool error = false;
		void run() {
			process.counter = 0;

			for(unsigned int i=0; i<sizeof(reqs)/sizeof(reqs[0]); i++) {
				reqs[i].init();
				reqs[i].success = -1;
				if(!DummyProcess<Os>::instance.submitTimeout(reqs + i, 100 + 10 * i)) {
					error = true;
					return;
				}
			}

			auto* s1 = Os::select(&reqs[0], &reqs[1], &reqs[2]);

			if(!s1 || static_cast<Req*>(s1)->success != (int)Os::IoChannel::Job::Result::TimedOut) {
				error = true;
				return;
			}

			process.counter = 3;
			process.submit(static_cast<Req*>(s1));

			for(unsigned int i=0; i<sizeof(reqs)/sizeof(reqs[0]); i++) {
				reqs[i].wait();

				if(reqs[i].success != (int)Os::IoChannel::Job::Result::Done) {
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
