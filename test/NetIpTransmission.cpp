/*
 * NetPacketAllocation.cpp
 *
 *  Created on: 2017.11.19.
 *      Author: tooma
 */

#include "common/TestNetDefinitions.h"

TEST_GROUP(NetPacketAllocation) {};

TEST(NetPacketAllocation, NoRoute) {
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

	task.start();
	Net::init();
	CommonTestUtils::start();
	CHECK(!task.error);
}


TEST(NetPacketAllocation, Unresolved) {
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

	task.start();
	Net::init();
	Net::addRoute(Net::Route(
	        AddressIp4::make(10, 10, 10, 0), 8,
	        AddressIp4::make(10, 10, 10, 10), Net::getIf<DummyIf<0>>()));
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(NetPacketAllocation, Successful) {
	struct Task: public TestTask<Task> {
		bool run() {
			Net::IpTransmission tx;
			tx.init();
			if(!tx.prepare(AddressIp4::make(10, 10, 10, 1), 6))
				return bad;

			// TODO immediate fall-through buffer allocation if there are no waiters.
/*			if(tx.shouldWait())
				return bad;

			if(tx.getError())
				return bad;

			tx.fill("foobar", 6);
			tx.send();*/
			return ok;
		}
	} task;

	task.start();
	Net::init();
	Net::addRoute(Net::Route(
	        AddressIp4::make(10, 10, 10, 0), 8,
	        AddressIp4::make(10, 10, 10, 10), Net::getIf<DummyIf<0>>())
	);
	Net::getIf<DummyIf<0>>()->getArpCache()->set(
			AddressIp4::make(10, 10, 10, 1),
			AddressEthernet::make(0x00, 0xAC, 0xCE, 0x55, 0x1B, 0x1E),
			INTPTR_MAX-1
	);
	CommonTestUtils::start();
	CHECK(!task.error);
}
