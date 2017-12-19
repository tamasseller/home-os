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

TEST(NetIcmpArp, ArpRequest) {
    struct Task: public TestTask<Task> {
        bool run() {
            Net::template getEthernetInterface<DummyIf>()->expectN(1,
                /*            dst                 |                src                | etherType */
				0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x08, 0x06,
                /* hwType | protoType |hSize|pSize|   opCode  |           sender MAC              */
                0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x02, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00,
                /*      sender IP     |          target MAC               |       target IP       */
                0x0a, 0x0a, 0x0a, 0x0a, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x0a, 0x0a, 0x0a, 0x01
            );

        	Net::template getEthernetInterface<DummyIf>()->receive(
				/*            dst                 |                src                | etherType */
				0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x06,
				/* hwType | protoType |hSize|pSize|   opCode  |           sender MAC              */
				0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x01, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e,
				/*      sender IP     |          target MAC               |       target IP       */
				0x0a, 0x0a, 0x0a, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x0a, 0x0a, 0x0a
        	);

        	Net::Os::sleep(1);

        	return ok;
        }
    } task;

    work(task);
}
