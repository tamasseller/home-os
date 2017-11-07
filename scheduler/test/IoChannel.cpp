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
	public:
		inline virtual ~Process() {}

		struct Job: Os::IoChannel::Job {
			Job *next, *prev;
			unsigned int x;
		};

		void reset() {
			counter = 0;
		}

	private:

		static void processWorkerIsr();

		pet::DoubleList<Job> requests;

		virtual bool addJob(Os::IoChannel::Job* job) {
			return requests.addBack(static_cast<Job*>(job));
		}

		virtual bool removeJob(Os::IoChannel::Job* job) {
			return requests.remove(static_cast<Job*>(job));
		}

		virtual bool hasJob() {
			return requests.front() != nullptr;
		}

		virtual void enableProcess() {
			CommonTestUtils::registerIrq(&Process::processWorkerIsr);
		}

		virtual void disableProcess() {
			CommonTestUtils::registerIrq(nullptr);
		}

		static unsigned int counter;
	} process;

	unsigned int Process::counter;

	void Process::processWorkerIsr() {
		Job* current = process.requests.front();

		current->x = counter++;

		process.jobDone(current);
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
