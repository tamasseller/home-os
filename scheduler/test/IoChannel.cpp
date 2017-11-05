/*
 * Io.cpp
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

		static unsigned int counter;
	public:
		inline virtual ~Process() {}

		struct Job: Os::IoChannel::Job {
			unsigned int x;
		};

		void reset() {
			counter = 0;
		}
	} process;

	unsigned int Process::counter;

	void Process::processWorkerIsr() {
		Job* current = static_cast<Job*>(process.currentJob());

		current->x = counter++;

		process.jobDone();
	}
}

TEST(IoChannel) {
	struct Task: public TestTask<Task> {
		void run() {
			Process::Job jobs[5];

			for(unsigned int i=0; i<sizeof(jobs)/sizeof(jobs[0]); i++) {
				jobs[i].x = -1;
				process.submit(jobs + i);
			}

			while(true) {
				bool done = true;
				for(unsigned int i=0; i<sizeof(jobs)/sizeof(jobs[0]); i++)
					if(jobs[i].x != i)
						done = false;

				if(done)
					break;
			}
		}
	} task;

	process.reset();
	task.start();
	CommonTestUtils::start();
}
