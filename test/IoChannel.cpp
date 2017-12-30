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
		bool run() {
			if(!postJobsNoTimeout()) return bad;

			Os::sleep(50);

			for(int i = 0; i < 5; i++)
				if(!jobs[i].isOccupied()) return bad;

			process.counter = 5;

			if(checkJobs() != 5) return bad;

			for(int i = 0; i < 5; i++)
				if(jobs[i].isOccupied()) return bad;

			for(int i = 0; i < 5; i++)
				if(jobs[i].count != 5 - i) return bad;

			return ok;
		}
	} task;

	Base::process.init();
	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(IoChannelTimeout) {
	struct Task: Base, public TestTask<Task> {
		bool run() {
			for(int i=sizeof(jobs)/sizeof(jobs[0]); i>=0; i--) {

				if(!postJobs()) return bad;

				process.counter = i;

				if(checkJobs() != i) return bad;

				return ok;
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
		bool run() {
			if(!postJobs()) return bad;

			Os::sleep(110);

			for(unsigned int i=0; i<sizeof(jobs)/sizeof(jobs[0]); i++)
				if(jobs[i].success != (int)Os::IoJob::Result::TimedOut)
					return bad;

			return ok;
		}
	} task;

	Base::process.init();
	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(IoChannelTimeoutCancel) {
	struct Task: Base, public TestTask<Task> {
		bool run() {
			if(!postJobs()) return bad;

			Os::sleep(40);

			for(unsigned int i=0; i<sizeof(jobs)/sizeof(jobs[0]); i++) {
				if(!jobs[i].isOccupied()) return bad;
				jobs[i].cancel();
			}

			Os::sleep(10);

			process.counter = 5;

			Os::sleep(50);

			for(unsigned int i=0; i<sizeof(jobs)/sizeof(jobs[0]); i++) {
				if(jobs[i].isOccupied()) return bad;
				if(jobs[i].success != (int)Os::IoJob::Result::Canceled)
					return bad;
			}

			return ok;
		}
	} task;

	Base::process.init();
	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(IoChannelMulti) {
	struct Task: Base, public TestTask<Task> {
		bool run() {
			MultiJob<3> multiJob;
			multiJob.launch(MultiJob<3>::entry);
			process.counter = 3;

			while(multiJob.idx){}

			for(int i=0; i<3; i++)
				if(multiJob.success[i] != (int)Os::IoJob::Result::Done) return bad;

			return ok;
		}
	} task;

	Base::process.init();
	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(IoChannelErrors) {
	struct Task: Base, public TestTask<Task> {
		bool run() {
			if(jobs[0].launchTimeout(Job::entry, 0)) return bad;

			if(jobs[0].launchTimeout(Job::entry, (uintptr_t)-1)) return bad;

			if(!jobs[0].launch(Job::entry)) return bad;

			if(jobs[0].launch(Job::entry)) return bad;

			if(jobs[0].launchTimeout(Job::entry, 10)) return bad;

			process.counter = 1;

			while(jobs[0].success != (int)Os::IoJob::Result::Done) {}

			return ok;
		}
	} task;

	Base::process.init();
	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(IoSemaphoreChannel) {
	struct Task: Base, public TestTask<Task> {
		bool run() {
			SemJob jobs[3];

			jobs[0].launch(SemJob::entry, 1);

			if(!jobs[0].done) return bad;	// 1

			jobs[0].launch(SemJob::entry, -1);

			if(!jobs[0].done) return bad;	// 0

			jobs[0].launch(SemJob::entry, -1);

			if(jobs[0].done) return bad;	// 0

			jobs[1].launch(SemJob::entry, 2);

			if(!jobs[1].done) return bad;	// 2
			if(!jobs[0].done) return bad;	// 1

			jobs[0].launch(SemJob::entry, -2);

			if(jobs[0].done) return bad;	// 1

			jobs[1].launch(SemJob::entry, -1);

			if(jobs[0].done) return bad;	// 1
			if(jobs[1].done) return bad;	// 1

			jobs[2].launch(SemJob::entry, 3);

			if(!jobs[2].done) return bad;	// 3
			if(!jobs[0].done) return bad;	// 1
			if(!jobs[1].done) return bad;	// 0

			return ok;
		}
	} task;

	Base::semProcess.init();
	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(IoSemaphoreChannelCancel) {
	struct Task: Base, public TestTask<Task> {
		bool run() {
			SemJob job;

			job.launch(SemJob::entry, -1);

			if(job.done) return bad;

			job.cancel();

			return ok;
		}
	} task;

	Base::semProcess.init();
	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(IoChannelComposite) {
	struct Task: Base, public TestTask<Task> {
		bool run() {
			CompositeJob jobs[3];

			jobs[0].launch(CompositeJob::entry, -2);
			jobs[1].launch(CompositeJob::entry, -1);
			jobs[2].launch(CompositeJob::entry, 1);

			if(jobs[0].stage != 0) return bad;
			if(jobs[1].stage != 0) return bad;
			if(jobs[2].stage != 1) return bad;
			if(!jobs[0].isOccupied()) return bad;
			if(!jobs[1].isOccupied()) return bad;
			if(!jobs[2].isOccupied()) return bad;

			process.counter = 1;

			Os::sleep(10);

			if(jobs[0].stage != 0) return bad;
			if(jobs[1].stage != 0) return bad;
			if(jobs[2].stage != 2) return bad;
			if(!jobs[0].isOccupied()) return bad;
			if(!jobs[1].isOccupied()) return bad;
			if(jobs[2].isOccupied()) return bad;

			jobs[2].launch(CompositeJob::entry, 1);

			if(jobs[0].stage != 1) return bad;
			if(jobs[1].stage != 0) return bad;
			if(jobs[2].stage != 1) return bad;
			if(!jobs[0].isOccupied()) return bad;
			if(!jobs[1].isOccupied()) return bad;
			if(!jobs[2].isOccupied()) return bad;

			process.counter = 2;

			Os::sleep(10);

			if(jobs[0].stage != 2) return bad;
			if(jobs[1].stage != 0) return bad;
			if(jobs[2].stage != 2) return bad;
			if(jobs[0].isOccupied()) return bad;
			if(!jobs[1].isOccupied()) return bad;
			if(jobs[2].isOccupied()) return bad;

			process.counter = 100;

			jobs[2].launch(CompositeJob::entry, 1);

			Os::sleep(10);

			if(jobs[1].stage != 2) return bad;
			if(jobs[1].isOccupied()) return bad;

			return ok;
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
        bool run() {
            CompositeJob job;

            job.launchTimeout(CompositeJob::entry, 10, 1);

            while(!job.timedOut) {}

            if(job.isOccupied()) return bad;

			return ok;
        }
    } task;

    Base::process.init();
    Base::semProcess.init();
    task.start();
    CommonTestUtils::start();
    CHECK(!task.error);
}

TEST(IoChannelCompositeTimeoutCumulative) {
    struct Task: Base, public TestTask<Task> {
        bool run() {
            CompositeJob jobs[2];

            jobs[0].launchTimeout(CompositeJob::entry, 50, -1);

            Os::sleep(5);

            if(!jobs[0].isOccupied()) return bad;

            jobs[1].launch(CompositeJob::entry, 1);

            Os::sleep(1);

            if(!jobs[0].isOccupied()) return bad;
            if(jobs[0].stage != 1) return bad;

            Os::sleep(50);

            if(jobs[0].isOccupied()) return bad;
            if(!jobs[0].timedOut) return bad;

			return ok;
        }
    } task;

    Base::process.init();
    Base::semProcess.init();
    task.start();
    CommonTestUtils::start();
    CHECK(!task.error);
}

