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

TEST(IoRequestAlreadyDone) {
	struct Task: public TestTask<Task>, Base  {
		bool run() {
			if(!postReqsNoTimeout()) return bad;

			for(unsigned int i=0; i<sizeof(reqs)/sizeof(reqs[0]); i++)
				if(!reqs[i].isOccupied()) return bad;

			process.counter = 3;

			Os::sleep(10);

			if(process.counter) return bad;

			for(unsigned int i=0; i<sizeof(reqs)/sizeof(reqs[0]); i++) {
				if(!reqs[i].isOccupied()) return bad;
				reqs[i].wait();
				if(reqs[i].isOccupied()) return bad;

				if(reqs[i].success != (int)Os::IoJob::Result::Done) return bad;
			}

			return ok;
		}
	} task;

	Base::process.init();
	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(IoRequestWaitTimeout) {
	struct Task: public TestTask<Task>, Base  {
		bool run() {
			if(!postReqsNoTimeout()) return bad;

			for(unsigned int i=0; i<sizeof(reqs)/sizeof(reqs[0]); i++) {
				bool ok = reqs[i].wait(10);

				if(ok) return bad;
			}

			process.counter = 5;

			for(unsigned int i=0; i<sizeof(reqs)/sizeof(reqs[0]); i++) {
				bool ok = reqs[i].wait(10);

				if(!ok || reqs[i].success != (int)Os::IoJob::Result::Done) return bad;
			}

			return ok;
		}
	} task;

	Base::process.init();
	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(IoRequestWaitNoTimeout) {
	struct Task: public TestTask<Task>, Base  {
		bool run() {
			if(!postReqsNoTimeout()) return bad;

			process.counter = 3;

			for(unsigned int i=0; i<sizeof(reqs)/sizeof(reqs[0]); i++) {
				reqs[i].wait();

				if(reqs[i].success != (int)Os::IoJob::Result::Done) return bad;
			}

			return ok;
		}
	} task;

	Base::process.init();
	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(IoRequestSelect) {
	struct Task: public TestTask<Task>, Base {
		bool run() {
			if(!postReqsNoTimeout()) return bad;

			process.counter = 3;

			auto* s1 = Os::select(&reqs[0], &reqs[1], &reqs[2]);

			if(s1 != &reqs[0] || reqs[0].success != (int)Os::IoJob::Result::Done) return bad;

			auto* s2 = Os::select(&reqs[1], &reqs[2]);

			if(s2 != &reqs[1] || reqs[1].success != (int)Os::IoJob::Result::Done) return bad;

			auto* s3 = Os::select(&reqs[2]);

			if(s3 != &reqs[2] || reqs[2].success != (int)Os::IoJob::Result::Done) return bad;

			return ok;
		}
	} task;

	Base::process.init();
	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(IoRequestIoTimeoutSelect) {
	struct Task: public TestTask<Task>, Base {
		bool run() {
			for(unsigned int i=0; i<sizeof(reqs)/sizeof(reqs[0]); i++) {
				reqs[i].init();
				reqs[i].success = -1;
				if(!reqs[i].launchTimeout(Req::entry, 100 + 10 * i)) return bad;
			}

			auto* s1 = Os::select(&reqs[0], &reqs[1], &reqs[2]);

			if(!s1 || static_cast<Req*>(s1)->success != (int)Os::IoJob::Result::TimedOut) return bad;

			process.counter = 3;
			static_cast<Req*>(s1)->launch(Req::entry);

			for(unsigned int i=0; i<sizeof(reqs)/sizeof(reqs[0]); i++) {
				reqs[i].wait();

				if(reqs[i].success != (int)Os::IoJob::Result::Done) return bad;
			}

			return ok;
		}
	} task;

	Base::process.init();
	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(IoRequestProtection) {
	struct Task: public TestTask<Task>, Base  {
		bool run() {
			if(!postReqsNoTimeout()) return bad;

			if(reqs[0].launch(Req::entry)) return bad;
			if(reqs[1].launchTimeout(Req::entry, 10)) return bad;
			if(reqs[2].launchTimeout(Req::entry, 0)) return bad;

			for(unsigned int i=0; i<sizeof(reqs)/sizeof(reqs[0]); i++)
				if(!reqs[i].isOccupied()) return bad;

			process.counter = 3;

			Os::sleep(10);

			if(reqs[0].launch(Req::entry)) return bad;
			if(reqs[1].launchTimeout(Req::entry, 10)) return bad;
			if(reqs[2].launchTimeout(Req::entry, 0)) return bad;

			for(unsigned int i=0; i<sizeof(reqs)/sizeof(reqs[0]); i++)
				reqs[i].wait();

			if(!reqs[0].launch(Req::entry)) return bad;
			if(!reqs[1].launchTimeout(Req::entry, 10)) return bad;
			if(reqs[2].launchTimeout(Req::entry, 0)) return bad;

			reqs[0].cancel();
			reqs[1].cancel();

			return ok;
		}
	} task;

	Base::process.init();
	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}


TEST(IoRequestMulti) {
	struct Task: Base, public TestTask<Task> {
		bool run() {
			Os::IoRequest<MultiJob<3>> multiReq;
			multiReq.init();

			multiReq.launch(multiReq.entry);
			process.counter = 3;

			multiReq.wait();

			for(int i=0; i<3; i++)
				if(multiReq.success[i] != (int)Os::IoJob::Result::Done) return bad;

			return ok;
		}
	} task;

	Base::process.init();
	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(IoRequestComposite) {
	struct Task: Base, public TestTask<Task> {
		bool run() {
			Os::IoRequest<CompositeJob> compReq[2];

			compReq[0].init();
			compReq[0].launch(CompositeJob::entry, -1);
			process.counter = 100;

			if(compReq[0].wait(10))return bad;

			compReq[1].init();
			compReq[1].launch(CompositeJob::entry, 1);

			compReq[0].wait();
			compReq[1].wait();

			return ok;
		}
	} task;

	Base::semProcess.init();
	Base::process.init();
	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(IoRequestMultiSelect) {
	struct Task: Base, public TestTask<Task> {
		bool run() {
			Os::IoRequest<MultiJob<3>> multiReq[2];

			for(auto &x: multiReq) {
				x.init();
				x.launch(x.entry);
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
					if(x.success[i] != (int)Os::IoJob::Result::Done) return bad;

			return ok;
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
		OsRrPrio::Mutex &m;
		bool run() {
			m.lock();

			DummyProcess<OsRrPrio>::instance.counter = 0;

			if(!postReqsNoTimeout()) return bad;

			for(auto& x: reqs) {
				x.wait(3);

				DummyProcess<OsRrPrio>::instance.counter = 1;

                x.wait();

				if(x.success != (int)Os::IoJob::Result::Done) return bad;
			}

			m.unlock();

			return ok;
		}

		Tlow(OsRrPrio::Mutex &m): m(m) {}
	} tLow(m);

	struct Thigh: public TestTask<Thigh, OsRrPrio> {
		OsRrPrio::Mutex &m;
		bool run() {
			OsRrPrio::sleep(1);

			m.lock();
			m.unlock();

			return ok;
		}

		Thigh(OsRrPrio::Mutex &m): m(m) {}
	} tHigh(m);

	DummyProcess<OsRrPrio>::instance.init();
	m.init();
	tLow.start(uint8_t(1));
	tHigh.start(uint8_t(0));
	CommonTestUtils::start<OsRrPrio>();
	CHECK(!tLow.error);
}

TEST(IoRequestCompositeTimeout) {
    struct Task: Base, public TestTask<Task> {
        bool run() {
        	Os::IoRequest<CompositeJob> req;
            req.init();

            req.launchTimeout(req.entry, 10, 1);

            req.wait();

            if(req.getResult() == Os::IoJob::Result::TimedOut)
            	return bad;

            return ok;
        }
    } task;

    Base::process.init();
    Base::semProcess.init();
    task.start();
    CommonTestUtils::start();
    CHECK(!task.error);
}
