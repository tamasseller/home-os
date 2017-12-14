/*
 * NetIcmp.cpp
 *
 *  Created on: 2017.12.14.
 *      Author: tooma
 */

#include "common/TestNetDefinitions.h"

using Net = Net64;

TEST_GROUP(NetIcmpArp) {
	template<class Task>
	static inline void work(Task& task)
	{
		task.start();
		Net::init(NetBuffers<Net>::buffers);

		Net::addRoute(
				typename Net::Route(
					Net::template getEthernetInterface<DummyIf>(),
					AddressIp4::make(10, 10, 10, 10),
					8
				),
				true
		);

		Net::template getEthernetInterface<DummyIf>()->getArpCache()->set(
				AddressIp4::make(10, 10, 10, 1),
				AddressEthernet::make(0x00, 0xAC, 0xCE, 0x55, 0x1B, 0x1E),
				32767
		);

		CommonTestUtils::start();
		CHECK(!task.error);
	}
};

TEST(NetIcmpArp, Ping) {
    struct Task: public TestTask<Task> {
        bool run() {
        	//Net::template getEthernetInterface<DummyIf>()->receive();
        	return ok;
        }
    } task;

    work(task);
}
