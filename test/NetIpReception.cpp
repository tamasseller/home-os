/*
 * NetIpReception.cpp
 *
 *  Created on: 2017.12.30.
 *      Author: tooma
 */

#ifndef NETIPRECEPTION_CPP_
#define NETIPRECEPTION_CPP_

#include "common/TestNetDefinitions.h"

using Net = Net64;
using Accessor = NetworkTestAccessor<Net>;

TEST_GROUP(NetIpReception)
{
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

TEST(NetIpReception, IpReceptionInvalid) {
    struct Task: public TestTask<Task> {
        bool run() {
            auto initialRxUsage = Accessor::pool.statRxUsed();

            Net::template getEthernetInterface<DummyIf>()->receive(
                0x55, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
                0x35
            );

            Net::Os::sleep(1);
            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;

            Net::template getEthernetInterface<DummyIf>()->receive(
                0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
                0x45
            );

            Net::Os::sleep(1);
            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;

            Net::template getEthernetInterface<DummyIf>()->receive(
                0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
                0x45, 0x00
            );

            Net::Os::sleep(1);
            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;

            Net::template getEthernetInterface<DummyIf>()->receive(
                0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
                /* bullsh |  length  */
                0x45, 0x00, 0x00, 0x22
            );

            Net::Os::sleep(1);
            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;

            Net::template getEthernetInterface<DummyIf>()->receive(
                0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
                /* bullsh |  length   | frag. id  */
                0x45, 0x00, 0x00, 0x22, 0x00, 0x00
            );

            Net::Os::sleep(1);
            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;

            Net::template getEthernetInterface<DummyIf>()->receive(
                0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
                /* bullsh |  length   | frag. id  | flags+off */
                0x45, 0x00, 0x00, 0x22, 0x00, 0x00, 0x40, 0x00
            );

            /*
             * First fragment
             */
            Net::Os::sleep(1);
            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;

            Net::template getEthernetInterface<DummyIf>()->receive(
                0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
                /* bullsh |  length   | frag. id  | flags+off */
                0x45, 0x00, 0x00, 0x22, 0x00, 0x00, 0x60, 0x00
            );

            /*
             * Internal fragment
             */
            Net::Os::sleep(1);
            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;

            Net::template getEthernetInterface<DummyIf>()->receive(
                0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
                /* bullsh |  length   | frag. id  | flags+off */
                0x45, 0x00, 0x00, 0x22, 0x00, 0x00, 0x60, 0x01
            );

            /*
             * Last fragment
             */
            Net::Os::sleep(1);
            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;

            Net::template getEthernetInterface<DummyIf>()->receive(
                0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
                /* bullsh |  length   | frag. id  | flags+off */
                0x45, 0x00, 0x00, 0x22, 0x00, 0x00, 0x40, 0x02
            );

            Net::Os::sleep(1);
            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;

            Net::template getEthernetInterface<DummyIf>()->receive(
                0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
                /* bullsh |  length   | frag. id  | flags+off | TTL */
                0x45, 0x00, 0x00, 0x22, 0x00, 0x00, 0x40, 0x00, 0x40
            );

            Net::Os::sleep(1);
            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;

            Net::template getEthernetInterface<DummyIf>()->receive(
                /*            dst                 |                src                | etherType */
                0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
                /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
                0x45, 0x00, 0x00, 0x22, 0x00, 0x00, 0x40, 0x00, 0x40, 0x01, 0x12
            );

            Net::Os::sleep(1);
            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;

            Net::template getEthernetInterface<DummyIf>()->receive(
                /*            dst                 |                src                | etherType */
                0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
                /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
                0x45, 0x00, 0x00, 0x22, 0x00, 0x00, 0x40, 0x00, 0x40, 0x01, 0x12, 0xbd,
                /* source IP address  | destination IP address */
                0x0a, 0x0a, 0x0a, 0x1, 0x0a, 0x0a
            );

            Net::Os::sleep(1);
            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;

            /*
             * Truncated content.
             */
            Net::template getEthernetInterface<DummyIf>()->receive(
                /*            dst                 |                src                | etherType */
                0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
                /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
                0x45, 0x00, 0x00, 0x22, 0x00, 0x00, 0x40, 0x00, 0x40, 0x01, 0x12, 0xbd,
                /* source IP address  | destination IP address */
                0x0a, 0x0a, 0x0a, 0x1, 0x0a, 0x0a, 0x0a, 0x0a,
                /* type | code | checksum  |     id    |  seqnum   | */
                0x08,     0x00,  0xc0, 0xb9, 0x00, 0x01, 0x00, 0x01
            );

            Net::Os::sleep(1);
            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;

            /*
             * Truncated header (should be 24 bytes)
             */
            Net::template getEthernetInterface<DummyIf>()->receive(
                /*            dst                 |                src                | etherType */
                0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
                /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
                0x46, 0x00, 0x00, 0x22, 0x00, 0x00, 0x40, 0x00, 0x40, 0x01, 0xef, 0x01,
                /* source IP address  | destination IP address */
                0x0a, 0x0a, 0x0a, 0x1, 0x0a, 0x0a, 0x0a, 0x0a
            );

            /*
             * Bad checksum
             */
            Net::template getEthernetInterface<DummyIf>()->receive(
                /*            dst                 |                src                | etherType */
                0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
                /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
                0x45, 0x00, 0x00, 0x22, 0x00, 0x00, 0x40, 0x00, 0x40, 0x01, 0xf0, 0x01,
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

            /*
             * Unassigned protocol number.
             */
            Net::template getEthernetInterface<DummyIf>()->receive(
                0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
                /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
                0x45, 0x00, 0x00, 0x22, 0x00, 0x00, 0x40, 0x00, 0x40, 0xfc, 0x12, 0xbd,
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

            /*
             * Not for us.
             */
            Net::template getEthernetInterface<DummyIf>()->receive(
                /*            dst                 |                src                | etherType */
                0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
                /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
                0x45, 0x00, 0x00, 0x22, 0x00, 0x00, 0x40, 0x00, 0x40, 0x01, 0x12, 0xbe,
                /* source IP address  | destination IP address */
                0x0a, 0x0a, 0x0a, 0x1, 0x0a, 0x0a, 0x0a, 0x09,
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


#endif /* NETIPRECEPTION_CPP_ */
