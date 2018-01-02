/*
 * NetUdp.cpp
 *
 *  Created on: 2018.01.02.
 *      Author: tooma
 */

#include "common/TestNetDefinitions.h"

using Net = Net64;
using Accessor = NetworkTestAccessor<Net>;

TEST_GROUP(NetUdp)
{
    template<class... C>
    static bool checkContent(Net::UdpReceiver &r, C... c) {
        const uint8_t expected[] = {static_cast<uint8_t>(c)...};

        for(uint8_t e: expected) {
            uint8_t a;
            if(!r.read8(a) || a != e)
                return false;
        }

        return true;
    }

    static inline void receiveSimple() {
        Net::template getEthernetInterface<DummyIf>()->receive(
            /*            dst                 |                src                | etherType */
            0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x08, 0x00,
            /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
            0x45, 0x00, 0x00, 0x22, 0x84, 0x65, 0x40, 0x00, 0x40, 0x11, 0x8e, 0x47,
            /* source IP address  | destination IP address */
            0x0a, 0x0a, 0x0a, 0x1, 0x0a, 0x0a, 0x0a, 0x0a,
            /* srcport|  dstport  | udplength | checksum */
            0x82, 0x3b, 0x04, 0xd2, 0x00, 0x0e, 0x19, 0x62,
            /* payload */
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

TEST(NetUdp, DropRawNoListenerAtAll) {
    struct Task: public TestTask<Task> {
        bool run() {
            auto initialRxUsage = Accessor::pool.statRxUsed();

            receiveSimple();

            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;
            return ok;
        }
    } task;

    work(task);
}

TEST(NetUdp, DropNoMatchingListener) {
    struct Task: public TestTask<Task> {
        bool run() {
            auto initialRxUsage = Accessor::pool.statRxUsed();

            Net::UdpReceiver r;

            r.init();
            r.receive(0xf001);

            receiveSimple();

            if(r.wait(1)) return Task::bad;

            r.close();

            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;
            return ok;
        }
    } task;

    work(task);
}

TEST(NetUdp, ReceiveSimple) {
    struct Task: public TestTask<Task> {
        bool run() {
            auto initialRxUsage = Accessor::pool.statRxUsed();

            Net::UdpReceiver r;

            r.init();
            r.receive(1234);

            receiveSimple();

            r.wait();

            if(!checkContent(r, 'f', 'o', 'o', 'b', 'a', 'r')) return Task::bad;

            if(r.getPeerAddress() != AddressIp4::make(10, 10, 10, 1)) return Task::bad;
            if(r.getPeerPort() != 0x823b) return Task::bad;

            r.close();

            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;
            return ok;
        }
    } task;

    work(task);
}

TEST(NetUdp, ReceiveBad) {
    struct Task: public TestTask<Task> {
        bool run() {
            auto initialRxUsage = Accessor::pool.statRxUsed();

            Net::UdpReceiver r;

            r.init();
            r.receive(1234);

            receiveSimple();

            /*
             * Checksum error
             */
            Net::template getEthernetInterface<DummyIf>()->receive(
                /*            dst                 |                src                | etherType */
                0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x08, 0x00,
                /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
                0x45, 0x00, 0x00, 0x22, 0x84, 0x65, 0x40, 0x00, 0x40, 0x11, 0x8e, 0x47,
                /* source IP address  | destination IP address */
                0x0a, 0x0a, 0x0a, 0x1, 0x0a, 0x0a, 0x0a, 0x0a,
                /* srcport|  dstport  | udplength | checksum */
                0x82, 0x3b, 0x04, 0xd2, 0x00, 0x0e, 0xf0, 0x01,
                /* payload */
                'f', 'o', 'o', 'b', 'a', 'r'
            );

            /*
             * Payload truncated
             */
            Net::template getEthernetInterface<DummyIf>()->receive(
                /*            dst                 |                src                | etherType */
                0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x08, 0x00,
                /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
                0x45, 0x00, 0x00, 0x1f, 0x84, 0x65, 0x40, 0x00, 0x40, 0x11, 0x8e, 0x4a,
                /* source IP address  | destination IP address */
                0x0a, 0x0a, 0x0a, 0x1, 0x0a, 0x0a, 0x0a, 0x0a,
                /* srcport|  dstport  | udplength | checksum */
                0x82, 0x3b, 0x04, 0xd2, 0x00, 0x0e, 0xf0, 0x01,
                /* payload */
                'f', 'o', 'o'
            );

            /*
             * Payload truncated
             */
            Net::template getEthernetInterface<DummyIf>()->receive(
                /*            dst                 |                src                | etherType */
                0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x08, 0x00,
                /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
                0x45, 0x00, 0x00, 0x1b, 0x84, 0x65, 0x40, 0x00, 0x40, 0x11, 0x8e, 0x4e,
                /* source IP address  | destination IP address */
                0x0a, 0x0a, 0x0a, 0x1, 0x0a, 0x0a, 0x0a, 0x0a,
                /* srcport|  dstport  | udplength | checksum */
                0x82, 0x3b, 0x04, 0xd2, 0x00, 0x0e, 0xf0
            );

            if(r.wait(1)) return Task::bad;

            r.close();

            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;
            return ok;
        }
    } task;

    work(task);
}

TEST(NetUdp, ReceiveMultiple) {
    struct Task: public TestTask<Task> {
        bool run() {
            auto initialRxUsage = Accessor::pool.statRxUsed();

            Net::UdpReceiver r;

            r.init();
            r.receive(1234);

            doIndirect([](){
            	for(int i=0; i<3; i++)
            		receiveSimple();
            });

            for(int i=0; i<3; i++) {
                r.wait();
                if(!checkContent(r, 'f', 'o', 'o', 'b', 'a', 'r')) return Task::bad;
                if(r.getPeerAddress() != AddressIp4::make(10, 10, 10, 1)) return Task::bad;
                if(r.getPeerPort() != 0x823b) return Task::bad;
            }

            r.close();

            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;
            return ok;
        }
    } task;

    work(task);
}

TEST(NetUdp, SendSimple) {
    struct Task: public TestTask<Task> {
        bool run() {
            typename Net::UdpTransmitter tx;
            tx.init(0x823b);

            if(!tx.prepare(AddressIp4::make(10, 10, 10, 1), 0x04d2, 6))
                return Task::bad;

            tx.wait();

            if(tx.fill("foobar", 6) != 6)
                return Task::bad;

			Net::template getEthernetInterface<DummyIf>()->expectN(1,
				/*            dst                 |                src                | etherType */
				0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x08, 0x00,
				/* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
				0x45, 0x00, 0x00, 0x22, 0x00, 0x00, 0x40, 0x00, 0x40, 0x11, 0x12, 0xad,
				/* source IP address  | destination IP address */
				0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x1,
	            /* srcport|  dstport  | udplength | checksum */
	            0x82, 0x3b, 0x04, 0xd2, 0x00, 0x0e, 0x19, 0x62,
	            /* payload */
	            'f', 'o', 'o', 'b', 'a', 'r'
			);

            if(!tx.send())
                return Task::bad;

            tx.wait();
            return Task::ok;
        }
    } task;

    work(task);
}
