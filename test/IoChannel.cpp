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
#include "common/SampleIoActivities.h"

using Os=OsRr;
using Base = DummyProcessJobsBase<Os>;

TEST(IoChannel) {
	struct Task: Base, public TestTask<Task> {
		bool error = false;
		void run() {
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

			for(int i = 0; i < 5; i++) {
				if(jobs[i].count != 5 - i) {
					error = true;
					return;
				}
			}
		}
	} task;

	Base::process.init();
	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(IoChannelTimeout) {
	struct Task: Base, public TestTask<Task> {
		bool error = false;
		void run() {
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

	Base::process.init();
	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(IoChannelTimeoutDoTimout) {
	struct Task: Base, public TestTask<Task> {
		bool error = false;
		void run() {
			if(!postJobs()) {
				error = true;
				return;
			}

			Os::sleep(110);

			for(unsigned int i=0; i<sizeof(jobs)/sizeof(jobs[0]); i++)
				if(jobs[i].success != (int)Os::IoJob::Result::TimedOut)
					error = true;
		}
	} task;

	Base::process.init();
	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(IoChannelTimeoutCancel) {
	struct Task: Base, public TestTask<Task> {
		bool error = false;
		void run() {
			if(!postJobs()) {
				error = true;
				return;
			}

			Os::sleep(40);

			for(unsigned int i=0; i<sizeof(jobs)/sizeof(jobs[0]); i++)
				jobs[i].cancel();

			Os::sleep(10);

			process.counter = 5;

			Os::sleep(50);

			for(unsigned int i=0; i<sizeof(jobs)/sizeof(jobs[0]); i++)
				if(jobs[i].success != (int)Os::IoJob::Result::Canceled)
					error = true;
		}
	} task;

	Base::process.init();
	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(IoChannelMulti) {
	struct Task: Base, public TestTask<Task> {
		bool error = false;
		void run() {
			MultiJob<3> multiJob;
			multiJob.start();
			process.counter = 3;

			while(multiJob.idx){}

			for(int i=0; i<3; i++)
				if(multiJob.success[i] != (int)Os::IoJob::Result::Done) {error = true; return;}
		}
	} task;

	Base::process.init();
	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(IoChannelErrors) {
	struct Task: Base, public TestTask<Task> {
		bool error = false;
		void run() {
			if(jobs[0].startTimeout(0)) {
				error = true;
				return;
			}

			if(jobs[0].startTimeout((uintptr_t)-1)) {
				error = true;
				return;
			}

			if(!jobs[0].start()) {
				error = true;
				return;
			}

			if(jobs[0].start()) {
				error = true;
				return;
			}

			process.counter = 1;

			while(jobs[0].success != (int)Os::IoJob::Result::Done) {}
		}
	} task;

	Base::process.init();
	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(IoSemaphoreChannel) {
	struct Task: Base, public TestTask<Task> {
		bool error = false;
		void run() {
			SemJob jobs[3];

			jobs[0].start(1);

			if(!jobs[0].done) {error = true; return;}	// 1

			jobs[0].start(-1);

			if(!jobs[0].done) {error = true; return;}	// 0

			jobs[0].start(-1);

			if(jobs[0].done) {error = true; return;}	// 0

			jobs[1].start(2);

			if(!jobs[1].done) {error = true; return;}	// 2
			if(!jobs[0].done) {error = true; return;}	// 1

			jobs[0].start(-2);

			if(jobs[0].done) {error = true; return;}	// 1

			jobs[1].start(-1);

			if(jobs[0].done) {error = true; return;}	// 1
			if(jobs[1].done) {error = true; return;}	// 1

			jobs[2].start(3);

			if(!jobs[2].done) {error = true; return;}	// 3
			if(!jobs[0].done) {error = true; return;}	// 1
			if(!jobs[1].done) {error = true; return;}	// 0
		}
	} task;

	Base::semProcess.init();
	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(IoChannelComposite) {
	struct Task: Base, public TestTask<Task> {
		bool error = false;
		void run() {
			CompositeJob jobs[3];

			jobs[0].start(-2, 0);
			jobs[1].start(-1, 0);
			jobs[2].start(1, 0);

			if(jobs[0].stage != 0) {error = true; return;}
			if(jobs[1].stage != 0) {error = true; return;}
			if(jobs[2].stage != 1) {error = true; return;}

			process.counter = 1;

			Os::sleep(10);

			if(jobs[2].stage != 2) {error = true; return;}

			jobs[2].start(1, 0);

			if(jobs[0].stage != 1) {error = true; return;}
			if(jobs[2].stage != 1) {error = true; return;}

			process.counter = 2;

			Os::sleep(10);

			if(jobs[0].stage != 2) {error = true; return;}
			if(jobs[2].stage != 2) {error = true; return;}

			process.counter = 100;

			jobs[2].start(1, 0);

			Os::sleep(10);

			if(jobs[1].stage != 2) {error = true; return;}
		}
	} task;

	Base::process.init();
	Base::semProcess.init();
	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(IoChannelCompositeTimeout) {
    struct Task: Base, public TestTask<Task> {
        bool error = false;
        void run() {
            CompositeJob job;

            job.start(1, 10);

            while(!job.timedOut) {}
        }
    } task;

    Base::process.init();
    Base::semProcess.init();
    task.start();
    CommonTestUtils::start();
    CHECK(!task.error);
}
