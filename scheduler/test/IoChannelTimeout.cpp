/*
 * IoChannelTimeout.cpp
 *
 *  Created on: 2017.11.05.
 *      Author: tooma
 */

#include "common/CommonTestUtils.h"

using Os=OsRr;

namespace {
	class Process: public Os::IoChannel {
		static void processWorkerIsr();

		virtual void enableProcess() {
			CommonTestUtils::registerIrq(&Process::processWorkerIsr);
		}

		virtual void disableProcess() {
			CommonTestUtils::registerIrq(nullptr);
		}

	public:
		inline virtual ~Process() {}

		struct Job: Os::IoChannel::Job {
			int success;

			static void writeResult(Os::IoChannel::Job* item, Os::IoChannel::Job::Result result) {
				Job* job = static_cast<Job*>(item);
				job->success = (int)result;
			}

		public:
			inline Job(): Os::IoChannel::Job(&Job::writeResult), success(-1) {}
		};
	} process;

	volatile unsigned int counter;

	void Process::processWorkerIsr() {
		if(counter) {
			counter--;
			process.jobDone();
		}
	}

	struct Base {
		Process::Job jobs[5];

		bool postJobs()
		{
			bool ret = true;

			for(unsigned int i=0; i<sizeof(jobs)/sizeof(jobs[0]); i++) {
				jobs[i].success = -1;

				if(!process.submitTimeout(jobs + i, 100))
					ret = false;
			}

			return ret;
		}

		int checkJobs()
		{
			int ret;

			while(true) {
				bool done = true;
				ret = 0;

				for(unsigned int i=0; i<sizeof(jobs)/sizeof(jobs[0]); i++) {
					if(jobs[i].success == -1)
						done = false;
					else if(jobs[i].success == (int)Os::IoChannel::Job::Result::Done)
						ret++;
				}

				if(done)
					break;
			}

			return ret;
		}
	};
}

TEST(IoChannelTimeout) {
	struct Task: Base, public TestTask<Task> {
		bool error = false;
		void run() {
			counter = 0;
			for(int i=sizeof(jobs)/sizeof(jobs[0]); i>=0; i--) {

				if(!postJobs()) {
					error = true;
					return;
				}

				counter = i;

				if(checkJobs() != i) {
					error = true;
					return;
				}
			}
		}
	} task;

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(IoChannelTimeoutPostpone) {
	struct Task: Base, public TestTask<Task> {
		bool error = false;
		void run() {
			counter = 0;

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

			counter = 5;

			if(checkJobs() != 5)
				error = true;
		}
	} task;

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(IoChannelTimeoutDoTimout) {
	struct Task: Base, public TestTask<Task> {
		bool error = false;
		void run() {
			counter = 0;

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

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}


TEST(IoChannelTimeoutCancel) {
	struct Task: Base, public TestTask<Task> {
		bool error = false;
		void run() {
			counter = 0;

			if(!postJobs()) {
				error = true;
				return;
			}

			Os::sleep(40);

			for(unsigned int i=0; i<sizeof(jobs)/sizeof(jobs[0]); i++)
				process.cancel(jobs + i);

			Os::sleep(10);

			counter = 5;

			Os::sleep(50);

			for(unsigned int i=0; i<sizeof(jobs)/sizeof(jobs[0]); i++)
				if(jobs[i].success != (int)Os::IoChannel::Job::Result::Canceled)
					error = true;
		}
	} task;

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}
