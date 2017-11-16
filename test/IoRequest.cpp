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
#include "common/DummyProcess.h"

using Os=OsRr;
using Base = DummyProcessJobsBase<Os>;

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

	Base::process.init();
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

	Base::process.init();
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

	Base::process.init();
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

	Base::process.init();
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
				reqs[i].prepare();
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
			static_cast<Req*>(s1)->prepare();
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

	Base::process.init();
	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(IoRequestMulti) {
	struct Task: Base, public TestTask<Task> {
		bool error = false;
		void run() {
			Os::IoRequest<MultiJob<3>> multiReq;
			multiReq.init();
			multiReq.prepare();
			process.counter = 0;
			process.submit(&multiReq);
			process.counter = 3;

			multiReq.wait();

			for(int i=0; i<3; i++)
				CHECK(multiReq.success[i] == (int)Process::Job::Result::Done);
		}
	} task;

	Base::process.init();
	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(IoRequestMultiSelect) {
	struct Task: Base, public TestTask<Task> {
		bool error = false;
		void run() {
			Os::IoRequest<MultiJob<3>> multiReq[2];

			process.counter = 0;

			for(auto &x: multiReq) {
				x.init();
				x.prepare();
				process.submit(&x);
			}

			process.counter = 6;

			auto ret = Os::select(&multiReq[0], &multiReq[1]);

			CHECK(ret);

			if(ret == multiReq)
				multiReq[1].wait();
			else
				multiReq[0].wait();

			for(auto &x: multiReq)
				for(int i=0; i<3; i++)
					CHECK(x.success[i] == (int)Process::Job::Result::Done);
		}
	} task;

	Base::process.init();
	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(IoRequestVsMutexPrioChange)
{
	OsRrPrio::Mutex m;

	struct Tlow: public TestTask<Tlow, OsRrPrio>, DummyProcessJobsBase<OsRrPrio> {
		bool error = false;
		OsRrPrio::Mutex &m;
		void run() {
			m.lock();

			DummyProcess<OsRrPrio>::instance.counter = 0;

			if(!postReqsNoTimeout()) {
				error = true;
				return;
			}

			DummyProcess<OsRrPrio>::instance.counter = 1;

			for(auto& x: reqs) {
				x.wait(3);

				DummyProcess<OsRrPrio>::instance.counter = 1;

				Os::sleep(2);

				if(x.success != (int)Os::IoChannel::Job::Result::Done) {
					error = true;
					return;
				}
			}

			m.unlock();
		}

		Tlow(OsRrPrio::Mutex &m): m(m) {}
	} tLow(m);

	struct Thigh: public TestTask<Thigh, OsRrPrio> {
		bool error = false;
		OsRrPrio::Mutex &m;
		void run() {
			OsRrPrio::sleep(1);
			m.lock();
			m.unlock();
		}

		Thigh(OsRrPrio::Mutex &m): m(m) {}
	} tHigh(m);

	DummyProcess<OsRrPrio>::instance.init();
	m.init();
	tLow.start(1);
	tHigh.start(0);
	CommonTestUtils::start<OsRrPrio>();
	CHECK(!tLow.error);
}

