/*
 * NetIcmp.cpp
 *
 *  Created on: 2017.12.21.
 *      Author: tooma
 */

#include "common/TestNetDefinitions.h"

using Net = Net64;
using Accessor = NetworkTestAccessor<Net>;

TEST_GROUP(NetIcmpIcmp)
{
	static inline void expectReply(uint16_t n) {
        Net::template getEthernetInterface<DummyIf>()->expectN(n,
			/*            dst                 |                src                | etherType */
			0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x08, 0x00,
			/* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
			0x45, 0x00, 0x00, 0x22, 0x00, 0x00, 0x40, 0x00, 0xff, 0x01, 0x53, 0xbc,
			/* source IP address  | destination IP address */
			0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x1,
			/* type | code | checksum  |     id    |  seqnum   | */
			0x00,     0x00,  0xc8, 0xb9, 0x00, 0x01, 0x00, 0x01,
			/* data */
			'f', 'o', 'o', 'b', 'a', 'r'
        );
	}

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

TEST(NetIcmpIcmp, IcmpRequestSimple) {
    struct Task: public TestTask<Task> {
        bool run() {
            auto initialRxUsage = Accessor::pool.statRxUsed();

        	expectReply(1);

			Net::template getEthernetInterface<DummyIf>()->receive(
				/*            dst                 |                src                | etherType */
				0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
				/* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
				0x45, 0x00, 0x00, 0x22, 0x00, 0x00, 0x40, 0x00, 0x40, 0x01, 0x12, 0xbd,
				/* source IP address  | destination IP address */
				0x0a, 0x0a, 0x0a, 0x1, 0x0a, 0x0a, 0x0a, 0x0a,
				/* type | code | checksum  |     id    |  seqnum   | */
				0x08,     0x00,  0xc0, 0xb9, 0x00, 0x01, 0x00, 0x01,
				/* data */
				'f', 'o', 'o', 'b', 'a', 'r'
			);

        	Net::Os::sleep(1);

            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;
        	return ok;
        }
    } task;

    work(task);
}

TEST(NetIcmpIcmp, IcmpRequestMultiple) {
    struct Task: public TestTask<Task> {
        bool run() {
            auto initialRxUsage = Accessor::pool.statRxUsed();

        	expectReply(3);

        	doIndirect([](){
        		for(int i=0; i<3; i++) {
					Net::template getEthernetInterface<DummyIf>()->receive(
						/*            dst                 |                src                | etherType */
						0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
						/* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
						0x45, 0x00, 0x00, 0x22, 0x00, 0x00, 0x40, 0x00, 0x40, 0x01, 0x12, 0xbd,
						/* source IP address  | destination IP address */
						0x0a, 0x0a, 0x0a, 0x1, 0x0a, 0x0a, 0x0a, 0x0a,
						/* type | code | checksum  |     id    |  seqnum   | */
						0x08,     0x00,  0xc0, 0xb9, 0x00, 0x01, 0x00, 0x01,
						/* data */
						'f', 'o', 'o', 'b', 'a', 'r'
					);
        		}
        	});

        	Net::Os::sleep(1);

            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;
        	return ok;
        }
    } task;

    work(task);
}
