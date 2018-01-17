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
    template<class... C>
    static bool checkContent(Net::IpReceiver &r, C... c) {
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
            0x45, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x40, 0x00, 0x40, 0xfe, 0x11, 0xc8,
            /* source IP address  | destination IP address */
            0x0a, 0x0a, 0x0a, 0x1, 0x0a, 0x0a, 0x0a, 0x0a,
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

TEST(NetIpReception, DropRawNoListener) {
    struct Task: public TestTask<Task> {
        bool run() {
            auto initialRxUsage = Accessor::pool.statRxUsed();

            receiveSimple();

            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;
            if(Net::getCounterStats().ip.inputReceived != 1) return Task::bad;
            return ok;
        }
    } task;

    work(task);
}

TEST(NetIpReception, ReceiveRawWithListener) {
    struct Task: public TestTask<Task> {
        bool run() {
            auto initialRxUsage = Accessor::pool.statRxUsed();

            Net::IpReceiver r;

            r.init();
            r.receive();

            receiveSimple();

            r.wait();

            if(r.getPeerAddress() != AddressIp4::make(10, 10, 10, 1)) return Task::bad;
            if(r.getProtocol() != 0xfe) return Task::bad;
            if(!checkContent(r, 'f', 'o', 'o', 'b', 'a', 'r')) return Task::bad;

            r.close();

            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;
            if(Net::getCounterStats().ip.inputReceived != 1) return Task::bad;
            return ok;
        }
    } task;

    work(task);
}

TEST(NetIpReception, ReceiveRawRestart) {
    struct Task: public TestTask<Task> {
        bool run() {
            auto initialRxUsage = Accessor::pool.statRxUsed();

            Net::IpReceiver r;

            r.init();

            for(int i=0; i<3; i++) {
                r.receive();

                receiveSimple();

                r.wait();

                if(r.getPeerAddress() != AddressIp4::make(10, 10, 10, 1)) return Task::bad;
                if(r.getProtocol() != 0xfe) return Task::bad;
                if(!checkContent(r, 'f', 'o', 'o', 'b', 'a', 'r')) return Task::bad;

                r.close();
            }

            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;
            if(Net::getCounterStats().ip.inputReceived != 3) return Task::bad;
            return ok;
        }
    } task;

    work(task);
}

TEST(NetIpReception, Misusage) {
    struct Task: public TestTask<Task> {
        bool run() {
            auto initialRxUsage = Accessor::pool.statRxUsed();

            Net::IpReceiver r;

            r.init();

            if(checkContent(r, 'f', 'o', 'o', 'b', 'a', 'r')) return Task::bad;

            r.receive();

            if(checkContent(r, 'f', 'o', 'o', 'b', 'a', 'r')) return Task::bad;

            receiveSimple();

            if(checkContent(r, 'f', 'o', 'o', 'b', 'a', 'r')) return Task::bad;

            r.wait();

            if(!checkContent(r, 'f', 'o', 'o', 'b', 'a', 'r')) return Task::bad;

            r.close();

            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;
            if(Net::getCounterStats().ip.inputReceived != 1) return Task::bad;
            return ok;
        }
    } task;

    work(task);
}


TEST(NetIpReception, ReceiveRawMultiple) {
    struct Task: public TestTask<Task> {
        bool run() {
            auto initialRxUsage = Accessor::pool.statRxUsed();

            Net::IpReceiver r;

            r.init();
            r.receive();

            doIndirect([](){
                receiveSimple();

                Net::template getEthernetInterface<DummyIf>()->receive(
                    /*            dst                 |                src                | etherType */
                    0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x08, 0x00,
                    /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
                    0x45, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x40, 0x00, 0x40, 0xfd, 0x11, 0xc8,
                    /* source IP address  | destination IP address */
                    0x0a, 0x0a, 0x0a, 0x02, 0x0a, 0x0a, 0x0a, 0x0a,
                    /* data */
                    'f', 'r', 'o', 'b', 'b', 'r'
                );

                for(int i=0; i<2; i++)
                    Net::template getEthernetInterface<DummyIf>()->receive(
                          /*            dst                 |                src                | etherType */
                          0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x08, 0x00,
                          /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
                          0x45, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x40, 0x00, 0x40, 0xfc, 0x11, 0xc8,
                          /* source IP address  | destination IP address */
                          0x0a, 0x0a, 0x0a, 0x03, 0x0a, 0x0a, 0x0a, 0x0a,
                          /* data */
                          '1', '2', '3', '4', '5', '6'
                    );
            });

            r.wait();

            if(r.getPeerAddress() != AddressIp4::make(10, 10, 10, 1)) return Task::bad;
            if(r.getProtocol() != 0xfe) return Task::bad;
            if(!checkContent(r, 'f', 'o', 'o', 'b', 'a', 'r')) return Task::bad;
            if(!r.atEop()) return Task::bad;

            r.wait();

            if(r.getPeerAddress() != AddressIp4::make(10, 10, 10, 2)) return Task::bad;
            if(r.getProtocol() != 0xfd) return Task::bad;
            if(!checkContent(r, 'f', 'r', 'o', 'b', 'b', 'r')) return Task::bad;
            if(!r.atEop()) return Task::bad;

            r.wait();

            if(r.getPeerAddress() != AddressIp4::make(10, 10, 10, 3)) return Task::bad;
            if(r.getProtocol() != 0xfc) return Task::bad;
            if(!checkContent(r, '1', '2', '3', '4', '5', '6')) return Task::bad;

            r.close();

            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;
            if(Net::getCounterStats().ip.inputReceived != 4) return Task::bad;
            if(Net::getCounterStats().ip.inputFormatError != 0) return Task::bad;
            return ok;
        }
    } task;

    work(task);
}

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
            if(Net::getCounterStats().ip.inputFormatError != 1) return Task::bad;

            Net::template getEthernetInterface<DummyIf>()->receive(
                0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
                0x45
            );

            Net::Os::sleep(1);
            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;
            if(Net::getCounterStats().ip.inputFormatError != 2) return Task::bad;

            Net::template getEthernetInterface<DummyIf>()->receive(
                0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
                0x45, 0x00
            );

            Net::Os::sleep(1);
            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;
            if(Net::getCounterStats().ip.inputFormatError != 3) return Task::bad;

            Net::template getEthernetInterface<DummyIf>()->receive(
                0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
                /* bullsh |  length  */
                0x45, 0x00, 0x00, 0x22
            );

            Net::Os::sleep(1);
            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;
            if(Net::getCounterStats().ip.inputFormatError != 4) return Task::bad;

            Net::template getEthernetInterface<DummyIf>()->receive(
                0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
                /* bullsh |  length   | frag. id  */
                0x45, 0x00, 0x00, 0x22, 0x00, 0x00
            );

            Net::Os::sleep(1);
            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;
            if(Net::getCounterStats().ip.inputFormatError != 5) return Task::bad;

            Net::template getEthernetInterface<DummyIf>()->receive(
                0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
                /* bullsh |  length   | frag. id  | flags+off */
                0x45, 0x00, 0x00, 0x22, 0x00, 0x00, 0x40, 0x00
            );

            Net::Os::sleep(1);
            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;
            if(Net::getCounterStats().ip.inputFormatError != 6) return Task::bad;
            if(Net::getCounterStats().ip.inputReassemblyRequired != 0) return Task::bad;

            /*
             * First fragment
             */
            Net::template getEthernetInterface<DummyIf>()->receive(
                0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
                /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
                0x45, 0x00, 0x00, 0x22, 0x00, 0x00, 0x60, 0x00, 0x40, 0x01, 0x12, 0xbe,
                /* source IP address  | destination IP address */
                0x0a, 0x0a, 0x0a, 0x1, 0x0a, 0x0a, 0x0a, 0x0a
            );

            Net::Os::sleep(1);
            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;
            if(Net::getCounterStats().ip.inputReassemblyRequired != 1) return Task::bad;
            if(Net::getCounterStats().ip.inputFormatError != 6) return Task::bad;

            /*
             * Internal fragment
             */
            Net::template getEthernetInterface<DummyIf>()->receive(
                0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
                /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
                0x45, 0x00, 0x00, 0x22, 0x00, 0x00, 0x60, 0x01, 0x40, 0x01, 0x12, 0xbe,
                /* source IP address  | destination IP address */
                0x0a, 0x0a, 0x0a, 0x1, 0x0a, 0x0a, 0x0a, 0x0a
            );

            Net::Os::sleep(1);
            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;
            if(Net::getCounterStats().ip.inputReassemblyRequired != 2) return Task::bad;
            if(Net::getCounterStats().ip.inputFormatError != 6) return Task::bad;

            /*
             * Last fragment
             */
            Net::template getEthernetInterface<DummyIf>()->receive(
                0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
                /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
                0x45, 0x00, 0x00, 0x22, 0x00, 0x00, 0x40, 0x02, 0x40, 0x01, 0x12, 0xbe,
                /* source IP address  | destination IP address */
                0x0a, 0x0a, 0x0a, 0x1, 0x0a, 0x0a, 0x0a, 0x0a
            );

            Net::Os::sleep(1);
            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;
            if(Net::getCounterStats().ip.inputReassemblyRequired != 3) return Task::bad;
            if(Net::getCounterStats().ip.inputFormatError != 6) return Task::bad;

            Net::template getEthernetInterface<DummyIf>()->receive(
                0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
                /* bullsh |  length   | frag. id  | flags+off | TTL */
                0x45, 0x00, 0x00, 0x22, 0x00, 0x00, 0x40, 0x00, 0x40
            );

            Net::Os::sleep(1);
            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;
            if(Net::getCounterStats().ip.inputFormatError != 7) return Task::bad;
            if(Net::getCounterStats().ip.inputReassemblyRequired != 3) return Task::bad;

            Net::template getEthernetInterface<DummyIf>()->receive(
                /*            dst                 |                src                | etherType */
                0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
                /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
                0x45, 0x00, 0x00, 0x22, 0x00, 0x00, 0x40, 0x00, 0x40, 0x01, 0x12
            );

            Net::Os::sleep(1);
            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;
            if(Net::getCounterStats().ip.inputFormatError != 8) return Task::bad;

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
            if(Net::getCounterStats().ip.inputFormatError != 9) return Task::bad;

            /*
             * Truncated content.
             */
            Net::template getEthernetInterface<DummyIf>()->receive(
                /*            dst                 |                src                | etherType */
                0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
                /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
                0x45, 0x00, 0x00, 0x22, 0x00, 0x00, 0x40, 0x00, 0x40, 0x11, 0x12, 0xad,
                /* source IP address  | destination IP address */
                0x0a, 0x0a, 0x0a, 0x1, 0x0a, 0x0a, 0x0a, 0x0a,
                /* srcport|  dstport  | udplength | checksum */
                0x08, 0x00, 0xc0, 0xb9, 0x00, 0x12, 0x00, 0x00
            );

            Net::Os::sleep(1);
            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;
            if(Net::getCounterStats().ip.inputFormatError != 10) return Task::bad;

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

            Net::Os::sleep(1);
            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;
            if(Net::getCounterStats().ip.inputFormatError != 11) return Task::bad;

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
            if(Net::getCounterStats().ip.inputForwardingRequired != 0) return Task::bad;
            if(Net::getCounterStats().ip.inputFormatError != 12) return Task::bad;

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
            if(Net::getCounterStats().ip.inputForwardingRequired != 1) return Task::bad;
            if(Net::getCounterStats().ip.inputFormatError != 12) return Task::bad;

            return ok;
        }
    } task;

    work(task);
}


#endif /* NETIPRECEPTION_CPP_ */
