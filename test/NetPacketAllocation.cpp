/*
 * NetPacketAllocation.cpp
 *
 *  Created on: 2017.11.19.
 *      Author: tooma
 */

#include "common/TestNetDefinitions.h"

TEST_GROUP(NetPacketAllocation) {

};

TEST(NetPacketAllocation, Simple) {
	struct Task: public TestTask<Task> {
		bool run() {
			Net::IpTransmission tx;
			tx.init();
			tx.prepare(AddressIp4::make(10, 10, 10, 10), 6);
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
