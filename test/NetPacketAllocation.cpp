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
		bool error = false;

		void run() {
			Net::UdpTransmission tx;
			tx.prepare(AddressIp4::make(10, 10, 10, 10), 1234, 5678, 6);
			tx.fill("foobar", 6);
			tx.send();
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
