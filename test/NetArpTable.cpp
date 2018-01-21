/*
 * NetArp.cpp
 *
 *  Created on: 2017.12.10.
 *      Author: tooma
 */

#include "common/CommonTestUtils.h"

#include "core/ArpTable.h"

using Os = OsRr;

TEST_GROUP(NetArpTable) {
	typedef ArpTable<Os, 3> Uut;

	class Job: public Os::IoJob {
		static bool done(Os::IoJob::Launcher* launcher, Os::IoJob* item, Os::IoJob::Result result)
		{
			auto self = static_cast<Job*>(item);

			switch(result) {
			case Os::IoJob::Result::Done:
				self->isDone = true;
				break;
			case Os::IoJob::Result::TimedOut:
				self->timedOut = true;
				break;
			case Os::IoJob::Result::Canceled:
				self->canceled = true;
				break;
			default:
				break;
			}

			return false;
		}

		static bool start(Os::IoJob::Launcher* launcher, Os::IoJob* item, Uut* uut, AddressIp4 addr) {
			auto self = static_cast<Job*>(item);
			self->data.ip = addr;
			launcher->launch(uut, &Job::done, &self->data);
			return true;
		}

	public:
		constexpr static auto &entry = start;

		Uut::Data data;

		bool isDone = false;
		bool timedOut = false;
		bool canceled = false;
	};
};

TEST(NetArpTable, DirectNotPresent) {
	Uut uut;

	struct Task: public TestTask<Task> {
		Uut &uut;
		bool run() {
			uut.init();

			AddressEthernet result = AddressEthernet::make(5, 6, 7, 8, 9, 10);
			if(uut.lookUp(AddressIp4::make(1, 2, 3, 4), result)) return bad;

			if(result != AddressEthernet::make(5, 6, 7, 8, 9, 10)) return bad;

			return ok;
		}

		Task(Uut& uut): uut(uut) {}
	} task(uut);

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(NetArpTable, DirectOverwrite) {
	Uut uut;

	struct Task: public TestTask<Task> {
		Uut &uut;
		bool run() {
			uut.init();

			uut.set(AddressIp4::make(1, 2, 3, 4), AddressEthernet::make('f', 'o', 'o', 'b', 'a', 'r'), 10);
			uut.set(AddressIp4::make(1, 2, 3, 4), AddressEthernet::make('f', 'r', 'o', 'b', 'b', 'd'), 10);

			AddressEthernet result = AddressEthernet::make(5, 6, 7, 8, 9, 10);
			if(!uut.lookUp(AddressIp4::make(1, 2, 3, 4), result)) return bad;

			if(result != AddressEthernet::make('f', 'r', 'o', 'b', 'b', 'd')) return bad;

			return ok;
		}

		Task(Uut& uut): uut(uut) {}
	} task(uut);

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(NetArpTable, DirectAlreadyPresent) {
	Uut uut;

	struct Task: public TestTask<Task> {
		Uut &uut;
		bool run() {
			uut.init();

			AddressEthernet result = AddressEthernet::make(5, 6, 7, 8, 9, 10);

			uut.set(AddressIp4::make(1, 2, 3, 4), AddressEthernet::make('f', 'o', 'o', 'b', 'a', 'r'), 10);

			if(!uut.lookUp(AddressIp4::make(1, 2, 3, 4), result)) return bad;

			if(result != AddressEthernet::make('f', 'o', 'o', 'b', 'a', 'r')) return bad;

			return ok;
		}

		Task(Uut& uut): uut(uut) {}
	} task(uut);

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(NetArpTable, DirectPresentNotAlone) {
	Uut uut;

	struct Task: public TestTask<Task> {
		Uut &uut;
		bool run() {
			uut.init();

			AddressEthernet result = AddressEthernet::make(5, 6, 7, 8, 9, 10);

			uut.set(AddressIp4::make(0, 1, 2, 3), AddressEthernet::make('s', 'o', 'm', 'e', 'a', 'd'), 10);
			uut.set(AddressIp4::make(1, 2, 3, 4), AddressEthernet::make('f', 'o', 'o', 'b', 'a', 'r'), 10);
			uut.set(AddressIp4::make(2, 3, 4, 5), AddressEthernet::make('s', 'o', 'm', 'e', 'a', 'd'), 10);

			if(!uut.lookUp(AddressIp4::make(1, 2, 3, 4), result)) return bad;

			if(result != AddressEthernet::make('f', 'o', 'o', 'b', 'a', 'r')) return bad;

			return ok;
		}

		Task(Uut& uut): uut(uut) {}
	} task(uut);

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(NetArpTable, DirectRemovedNotPresent) {
	Uut uut;

	struct Task: public TestTask<Task> {
		Uut &uut;
		bool run() {
			uut.init();

			uut.set(AddressIp4::make(1, 2, 3, 4), AddressEthernet::make('f', 'o', 'o', 'b', 'a', 'r'), 10);

			uut.kill(AddressIp4::make(1, 2, 3, 4));

			AddressEthernet result = AddressEthernet::make(5, 6, 7, 8, 9, 10);
			if(uut.lookUp(AddressIp4::make(1, 2, 3, 4), result)) return bad;

			if(result != AddressEthernet::make(5, 6, 7, 8, 9, 10)) return bad;

			return ok;
		}

		Task(Uut& uut): uut(uut) {}
	} task(uut);

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(NetArpTable, DirectAlreadyAddedViaEvent) {
	Uut uut;

	struct Task: public TestTask<Task> {
		Uut &uut;
		bool run() {
			uut.init();

			AddressEthernet result = AddressEthernet::make(5, 6, 7, 8, 9, 10);

			doIndirect([this](){
				uut.handleResolved(
						AddressIp4::make(1, 2, 3, 4),
						AddressEthernet::make('f', 'o', 'o', 'b', 'a', 'r'),
						100);
			});

			if(!uut.lookUp(AddressIp4::make(1, 2, 3, 4), result)) return bad;

			if(result != AddressEthernet::make('f', 'o', 'o', 'b', 'a', 'r')) return bad;

			return ok;
		}

		Task(Uut& uut): uut(uut) {}
	} task(uut);

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(NetArpTable, DirectEntryAgeOut) {
	Uut uut;

	struct Task: public TestTask<Task> {
		Uut &uut;
		bool run() {
			uut.init();

			uut.set(AddressIp4::make(1, 2, 3, 4), AddressEthernet::make('f', 'o', 'o', 'b', 'a', 'r'), 3);

			for(int i = 0; i<4; i++) {
				AddressEthernet result = AddressEthernet::make(5, 6, 7, 8, 9, 10);
				if(!uut.lookUp(AddressIp4::make(1, 2, 3, 4), result)) return bad;
				if(result != AddressEthernet::make('f', 'o', 'o', 'b', 'a', 'r')) return bad;

				doIndirect([this](){
					uut.ageContent();
				});
			}

			AddressEthernet result = AddressEthernet::make(5, 6, 7, 8, 9, 10);
			if(uut.lookUp(AddressIp4::make(1, 2, 3, 4), result)) return bad;
			if(result != AddressEthernet::make(5, 6, 7, 8, 9, 10)) return bad;

			return ok;
		}

		Task(Uut& uut): uut(uut) {}
	} task(uut);

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(NetArpTable, JobAlreadyPresent) {
	Uut uut;

	struct Task: public TestTask<Task> {
		Uut &uut;
		bool run() {
			uut.init();

			uut.set(AddressIp4::make(1, 2, 3, 4), AddressEthernet::make('f', 'o', 'o', 'b', 'a', 'r'), 10);

			Job job;

			job.launch(Job::entry, &uut, AddressIp4::make(1, 2, 3, 4));

			if(!job.isDone) return bad;
			if(job.data.mac != AddressEthernet::make('f', 'o', 'o', 'b', 'a', 'r')) return bad;

			return ok;
		}

		Task(Uut& uut): uut(uut) {}
	} task(uut);

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(NetArpTable, JobAlreadyAddedViaEvent) {
	Uut uut;

	struct Task: public TestTask<Task> {
		Uut &uut;
		bool run() {
			uut.init();

			doIndirect([this](){
				uut.handleResolved(
						AddressIp4::make(1, 2, 3, 4),
						AddressEthernet::make('f', 'o', 'o', 'b', 'a', 'r'),
						100);
			});

			Job job;
			job.launch(Job::entry, &uut, AddressIp4::make(1, 2, 3, 4));

			if(!job.isDone) return bad;
			if(job.data.mac != AddressEthernet::make('f', 'o', 'o', 'b', 'a', 'r')) return bad;

			return ok;
		}

		Task(Uut& uut): uut(uut) {}
	} task(uut);

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(NetArpTable, JobAddedLaterViaEvent) {
	Uut uut;

	struct Task: public TestTask<Task> {
		Uut &uut;
		bool run() {
			uut.init();

			Job job;
			job.launch(Job::entry, &uut, AddressIp4::make(1, 2, 3, 4));

			if(job.isDone) return bad;

			doIndirect([this](){
				uut.handleResolved(
						AddressIp4::make(1, 2, 3, 4),
						AddressEthernet::make('f', 'o', 'o', 'b', 'a', 'r'),
						100);
			});

			if(!job.isDone) return bad;
			if(job.data.mac != AddressEthernet::make('f', 'o', 'o', 'b', 'a', 'r')) return bad;

			return ok;
		}

		Task(Uut& uut): uut(uut) {}
	} task(uut);

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(NetArpTable, JobAgedAddedLaterViaEvent) {
	Uut uut;

	struct Task: public TestTask<Task> {
		Uut &uut;
		bool run() {
			uut.init();

			Job job;
			job.launch(Job::entry, &uut, AddressIp4::make(1, 2, 3, 4));

			if(job.isDone) return bad;

			doIndirect([this](){
				uut.ageContent();
			});

			if(job.isDone) return bad;

			doIndirect([this](){
				uut.handleResolved(
						AddressIp4::make(1, 2, 3, 4),
						AddressEthernet::make('f', 'o', 'o', 'b', 'a', 'r'),
						100);
			});

			if(!job.isDone) return bad;
			if(job.data.mac != AddressEthernet::make('f', 'o', 'o', 'b', 'a', 'r')) return bad;

			return ok;
		}

		Task(Uut& uut): uut(uut) {}
	} task(uut);

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(NetArpTable, JobAgedToTimeout) {
	Uut uut;

	struct Task: public TestTask<Task> {
		Uut &uut;
		bool run() {
			uut.init();

			Job job;
			job.launch(Job::entry, &uut, AddressIp4::make(1, 2, 3, 4));

			if(job.isDone) return bad;

			doIndirect([this](){
				uut.ageContent();
			});

			if(job.isDone) return bad;

			doIndirect([this](){
				uut.ageContent();
			});

			if(!job.isDone) return bad;
			if(job.data.mac != AddressEthernet::make(0, 0, 0, 0, 0, 0)) return bad;

			return ok;
		}

		Task(Uut& uut): uut(uut) {}
	} task(uut);

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(NetArpTable, JobAgedCanceledResolved) {
	Uut uut;

	struct Task: public TestTask<Task> {
		Uut &uut;
		bool run() {
			uut.init();

			Job job;
			job.launch(Job::entry, &uut, AddressIp4::make(1, 2, 3, 4));

			if(job.isDone) return bad;

			doIndirect([this](){
				uut.ageContent();
			});

			if(job.isDone) return bad;

			job.cancel();

			if(!job.canceled) return bad;

			doIndirect([this](){
				uut.handleResolved(
						AddressIp4::make(1, 2, 3, 4),
						AddressEthernet::make('f', 'o', 'o', 'b', 'a', 'r'),
						100);
			});

			if(job.isDone) return bad;
			if(job.data.ip != AddressIp4::make(1, 2, 3, 4)) return bad;

			return ok;
		}

		Task(Uut& uut): uut(uut) {}
	} task(uut);

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

