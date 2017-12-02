/*
 * NetIpTransmission.cpp
 *
 *  Created on: 2017.11.19.
 *      Author: tooma
 */

#include "common/TestNetDefinitions.h"

TEST_GROUP(NetIpTransmission) {
	template<bool addRoute, bool addArp, class Task>
	void work(Task& task)
	{
		task.start();
		Net::init(NetBuffers<Net>::buffers);

		if(addRoute) {
			Net::addRoute(Net::Route(
					AddressIp4::make(10, 10, 10, 0), 8,
					AddressIp4::make(10, 10, 10, 10), Net::getIf<DummyIf<0>>())
			);
		}

		if(addArp) {
			Net::getIf<DummyIf<0>>()->getArpCache()->set(
					AddressIp4::make(10, 10, 10, 1),
					AddressEthernet::make(0x00, 0xAC, 0xCE, 0x55, 0x1B, 0x1E),
					INTPTR_MAX-1
			);
		}

		CommonTestUtils::start();
		CHECK(!task.error);
	}
};

TEST(NetIpTransmission, NoRoute) {
	struct Task: public TestTask<Task> {
		bool run() {
			Net::IpTransmission tx;
			tx.init();
			if(tx.prepare(AddressIp4::make(10, 10, 10, 1), 6))
				return bad;

			if(tx.getError() != NetErrorStrings::noRoute)
				return bad;

			return ok;
		}
	} task;

	work<false, false>(task);
}


TEST(NetIpTransmission, Unresolved) {
	struct Task: public TestTask<Task> {
		bool run() {
			Net::IpTransmission tx;
			tx.init();
			if(!tx.prepare(AddressIp4::make(10, 10, 10, 1), 6))
				return bad;

			tx.wait();

			if(tx.getError() != NetErrorStrings::unresolved)
				return bad;

			tx.fill("foobar", 6);
			tx.send();
			return ok;
		}
	} task;

	work<true, false>(task);
}

TEST(NetIpTransmission, Resolved) {
	struct Task: public TestTask<Task> {
		bool run() {
			Net::IpTransmission tx;
			tx.init();
			if(!tx.prepare(AddressIp4::make(10, 10, 10, 1), 6))
				return bad;

			if(!tx.shouldWait())
				return bad;

			Net::getIf<DummyIf<0>>()->getArpCache()->set(
					AddressIp4::make(10, 10, 10, 1),
					AddressEthernet::make(0x00, 0xAC, 0xCE, 0x55, 0x1B, 0x1E),
					INTPTR_MAX-1
			);

			tx.wait();

			if(tx.getError())
				return bad;

/*			tx.fill("foobar", 6);
			tx.send();*/
			return ok;
		}
	} task;

	work<true, false>(task);
}


TEST(NetIpTransmission, Successful) {
	struct Task: public TestTask<Task> {
		bool run() {
			Net::IpTransmission tx;
			tx.init();
			if(!tx.prepare(AddressIp4::make(10, 10, 10, 1), 6))
				return bad;

			if(tx.shouldWait())
				return bad;

			if(tx.getError())
				return bad;

/*			tx.fill("foobar", 6);
			tx.send();*/
			return ok;
		}
	} task;

	work<true, true>(task);
}
