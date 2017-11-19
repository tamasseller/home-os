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
			auto ret = Net::prepareUdpPacket(AddressIp4::make(10, 10, 10, 10), 12345, 6);
			ret.packet->copyIn("foobar", 6);
			ret.send();
		}
	} task;

	task.start();
	Net::init();
	CommonTestUtils::start();
	CHECK(!task.error);
}
