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

			static void writeResult(Os::IoChannel::Job* item, bool result) {
				Job* job = static_cast<Job*>(item);

				job->success = result ? 1 : 0;
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

		void postJobs() {
			for(unsigned int i=0; i<sizeof(jobs)/sizeof(jobs[0]); i++) {
				jobs[i].success = -1;
				process.submitTimeout(jobs + i, 100);
			}
		}

		int checkJobs() {
			while(true) {
				bool done = true;
				int ret = 0;

				for(unsigned int i=0; i<sizeof(jobs)/sizeof(jobs[0]); i++) {
					if(jobs[i].success == -1)
						done = false;
					else if(jobs[i].success == 1)
						ret++;
				}

				if(done)
					return ret;
			}
		}
	};
}

TEST(IoChannelTimeout) {
	struct Task: Base, public TestTask<Task> {
		bool error = false;
		void run() {
			counter = 0;
			for(int i=sizeof(jobs)/sizeof(jobs[0]); i>=0; i--) {
				postJobs();
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
			postJobs();

			Os::sleep(50);

			for(unsigned int i=0; i<sizeof(jobs)/sizeof(jobs[0]); i++)
				if(jobs[i].success != -1)
					error = true;

			postJobs();

			Os::sleep(50);

			counter = 5;

			if(checkJobs() != 5)
				error = true;
		}
	} task;

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(IoChannelTimeoutTimout) {
	struct Task: Base, public TestTask<Task> {
		bool error = false;
		void run() {
			counter = 0;
			postJobs();

			Os::sleep(50);

			for(unsigned int i=0; i<sizeof(jobs)/sizeof(jobs[0]); i++)
				process.cancel(jobs + i);

			counter = 5;

			Os::sleep(50);

			for(unsigned int i=0; i<sizeof(jobs)/sizeof(jobs[0]); i++)
				if(jobs[i].success != -1)
					error = true;
		}
	} task;

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}
