/*
 * NetIcmp.cpp
 *
 *  Created on: 2017.12.21.
 *      Author: tooma
 */

#include "common/TestNetDefinitions.h"

using Net = Net64;
using Accessor = NetworkTestAccessor<Net>;

TEST_GROUP(NetIcmp)
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

    static inline void expectReplyLong(uint16_t n) {
        Net::template getEthernetInterface<DummyIf>()->expectN(n,
           /*            dst                 |                src                | etherType */
           0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x08, 0x00,
           /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
           0x45, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x00, 0xff, 0x01, 0x53, 0x9e,
           /* source IP address  | destination IP address */
           0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x1,
           /* type | code | checksum  |     id    |  seqnum   | */
           0x00,     0x00,  0x7b, 0x79, 0x00, 0x01, 0x00, 0x01,
           /* data */
           '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@',
           '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@'
        );
    }

    static inline void receiveRequest() {
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

    static inline void receiveReply() {
		Net::template getEthernetInterface<DummyIf>()->receive(
			/*            dst                 |                src                | etherType */
			0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
			/* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
			0x45, 0x00, 0x00, 0x22, 0x00, 0x00, 0x40, 0x00, 0x40, 0x01, 0x12, 0xbd,
			/* source IP address  | destination IP address */
			0x0a, 0x0a, 0x0a, 0x1, 0x0a, 0x0a, 0x0a, 0x0a,
			/* type | code | checksum  |     id    |  seqnum   | */
			0x00,     0x00,  0xc8, 0xb9, 0x00, 0x01, 0x00, 0x01,
			/* data */
			'f', 'o', 'o', 'b', 'a', 'r'
		);
    }

	static bool checkReplyContent(Net::IcmpReceiver &r) {
		const uint8_t expected[] = {
				0x45, 0x00, 0x00, 0x22, 0x00, 0x00, 0x40, 0x00, 0x40, 0x01, 0x12, 0xbd, 0x0a, 0x0a, 0x0a, 0x1,
				0x0a, 0x0a, 0x0a, 0x0a, 0x00, 0x00, 0xc8, 0xb9, 0x00, 0x01, 0x00, 0x01, 'f', 'o', 'o', 'b',
				'a', 'r'
		};

		int n=0;
		for(uint8_t c: expected) {
			uint8_t a;
			if(!r.read8(a) || a != c)
				return false;

			n++;
		}

		return n == sizeof(expected);
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

TEST(NetIcmp, IcmpRequestSimple) {
    struct Task: public TestTask<Task> {
        bool run() {
            auto initialRxUsage = Accessor::pool.statRxUsed();

        	expectReply(1);
        	receiveRequest();

        	Net::Os::sleep(1);

            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;
            if(Net::getCounterStats().icmp.inputReceived != 1) return Task::bad;
            if(Net::getCounterStats().icmp.inputFormatError != 0) return Task::bad;
            if(Net::getCounterStats().icmp.outputQueued != 1) return Task::bad;
            if(Net::getCounterStats().icmp.outputSent != 1) return Task::bad;
            if(Net::getCounterStats().icmp.pingRequests != 1) return Task::bad;
        	return ok;
        }
    } task;

    work(task);
}

TEST(NetIcmp, IcmpRequestMartian) {
    struct Task: public TestTask<Task> {
        bool run() {
            auto initialRxUsage = Accessor::pool.statRxUsed();

            Net::template getEthernetInterface<DummyIf>()->receive(
                /*            dst                 |                src                | etherType */
                0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
                /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
                0x45, 0x00, 0x00, 0x22, 0x00, 0x00, 0x40, 0x00, 0x40, 0x01, 0x13, 0xbd,
                /* source IP address  | destination IP address */
                0x09, 0x0a, 0x0a, 0x1, 0x0a, 0x0a, 0x0a, 0x0a,
                /* type | code | checksum  |     id    |  seqnum   | */
                0x08,     0x00,  0xc0, 0xb9, 0x00, 0x01, 0x00, 0x01,
                /* data */
                'f', 'o', 'o', 'b', 'a', 'r'
            );

            Net::Os::sleep(1);

            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;

            if(Net::getCounterStats().icmp.inputReceived != 1) return Task::bad;
            if(Net::getCounterStats().icmp.inputFormatError != 0) return Task::bad;
            if(Net::getCounterStats().icmp.pingRequests != 1) return Task::bad;
            if(Net::getCounterStats().ip.outputNoRoute != 1) return Task::bad;
            if(Net::getCounterStats().icmp.outputQueued != 0) return Task::bad;
            if(Net::getCounterStats().icmp.outputSent != 0) return Task::bad;

            return ok;
        }
    } task;

    work(task);
}

TEST(NetIcmp, IcmpRequestNoArp) {
    struct Task: public TestTask<Task> {
        bool run() {
            auto initialRxUsage = Accessor::pool.statRxUsed();

            Net::template getEthernetInterface<DummyIf>()->expectN(1,
                /*            dst                 |                src                | etherType */
                0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x08, 0x06,
                /* hwType | protoType |hSize|pSize|   opCode  |           sender MAC              */
                0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x01, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00,
                /*      sender IP     |          target MAC               |       target IP       */
                0x0a, 0x0a, 0x0a, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x0a, 0x0a, 0x02
            );

            Net::template getEthernetInterface<DummyIf>()->receive(
                /*            dst                 |                src                | etherType */
                0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
                /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
                0x45, 0x00, 0x00, 0x22, 0x00, 0x00, 0x40, 0x00, 0x40, 0x01, 0x12, 0xbc,
                /* source IP address  | destination IP address */
                0x0a, 0x0a, 0x0a, 0x2, 0x0a, 0x0a, 0x0a, 0x0a,
                /* type | code | checksum  |     id    |  seqnum   | */
                0x08,     0x00,  0xc0, 0xb9, 0x00, 0x01, 0x00, 0x01,
                /* data */
                'f', 'o', 'o', 'b', 'a', 'r'
            );

            Net::Os::sleep(40);

            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;

            if(Net::getCounterStats().icmp.inputReceived != 1) return Task::bad;
            if(Net::getCounterStats().icmp.inputFormatError != 0) return Task::bad;
            if(Net::getCounterStats().icmp.pingRequests != 1) return Task::bad;
            if(Net::getCounterStats().arp.requestSent != 1) return Task::bad;
            if(Net::getCounterStats().ip.outputArpFailed != 1) return Task::bad;
            if(Net::getCounterStats().icmp.outputQueued != 0) return Task::bad;
            if(Net::getCounterStats().icmp.outputSent != 0) return Task::bad;

            return ok;
        }
    } task;

    work(task);
}

TEST(NetIcmp, IcmpRequestLong) {
    struct Task: public TestTask<Task> {
        bool run() {
            auto initialRxUsage = Accessor::pool.statRxUsed();

            expectReplyLong(1);

            Net::template getEthernetInterface<DummyIf>()->receive(
                /*            dst                 |                src                | etherType */
                0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
                /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
                0x45, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x00, 0x40, 0x01, 0x12, 0x9f,
                /* source IP address  | destination IP address */
                0x0a, 0x0a, 0x0a, 0x1, 0x0a, 0x0a, 0x0a, 0x0a,
                /* type | code | checksum  |     id    |  seqnum   | */
                0x08,     0x00,  0x73, 0x79, 0x00, 0x01, 0x00, 0x01,
                /* data */
                '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@',
                '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@', '@'
            );

            Net::Os::sleep(1);

            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;
            return ok;
        }
    } task;

    work(task);
}

TEST(NetIcmp, IcmpRequestMultiple) {
    struct Task: public TestTask<Task> {
        bool run() {
            auto initialRxUsage = Accessor::pool.statRxUsed();

        	expectReply(3);

        	doIndirect([](){
        		for(int i=0; i<3; i++) {
        			receiveRequest();
        		}
        	});

        	Net::Os::sleep(1);

            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;

            if(Net::getCounterStats().icmp.inputReceived != 3) return Task::bad;
            if(Net::getCounterStats().icmp.inputFormatError != 0) return Task::bad;
            if(Net::getCounterStats().icmp.outputQueued != 3) return Task::bad;
            if(Net::getCounterStats().icmp.outputSent != 3) return Task::bad;
            if(Net::getCounterStats().icmp.pingRequests != 3) return Task::bad;

        	return ok;
        }
    } task;

    work(task);
}

TEST(NetIcmp, IcmpReceptionInvalid) {
    struct Task: public TestTask<Task> {
        bool run() {
            auto initialRxUsage = Accessor::pool.statRxUsed();

            /*
             * Truncated
             */
            Net::template getEthernetInterface<DummyIf>()->receive(
                /*            dst                 |                src                | etherType */
                0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
                /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
                0x45, 0x00, 0x00, 0x14, 0x00, 0x00, 0x40, 0x00, 0x40, 0x01, 0x12, 0xcb,
                /* source IP address  | destination IP address */
                0x0a, 0x0a, 0x0a, 0x1, 0x0a, 0x0a, 0x0a, 0x0a
            );

            if(Net::getCounterStats().icmp.inputReceived != 1) return Task::bad;
            if(Net::getCounterStats().icmp.inputFormatError != 1) return Task::bad;
            if(Net::getCounterStats().icmp.outputQueued != 0) return Task::bad;
            if(Net::getCounterStats().icmp.outputSent != 0) return Task::bad;
            if(Net::getCounterStats().icmp.pingRequests != 0) return Task::bad;

            /*
             * Invalid operation
             */
            Net::template getEthernetInterface<DummyIf>()->receive(
                /*            dst                 |                src                | etherType */
                0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
                /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
                0x45, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x40, 0x00, 0x40, 0x01, 0x12, 0xc3,
                /* source IP address  | destination IP address */
                0x0a, 0x0a, 0x0a, 0x1, 0x0a, 0x0a, 0x0a, 0x0a,
                /* type | code | checksum  |     id    |  seqnum   | */
                0xf0,     0x01,  0x0f, 0xfc, 0x00, 0x01, 0x00, 0x01
            );

            if(Net::getCounterStats().icmp.inputReceived != 2) return Task::bad;
            if(Net::getCounterStats().icmp.inputFormatError != 2) return Task::bad;
            if(Net::getCounterStats().icmp.outputQueued != 0) return Task::bad;
            if(Net::getCounterStats().icmp.outputSent != 0) return Task::bad;
            if(Net::getCounterStats().icmp.pingRequests != 0) return Task::bad;

            /*
             * Invalid ICMP checksum
             */
            Net::template getEthernetInterface<DummyIf>()->receive(
                /*            dst                 |                src                | etherType */
                0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
                /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
                0x45, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x40, 0x00, 0x40, 0x01, 0x12, 0xc3,
                /* source IP address  | destination IP address */
                0x0a, 0x0a, 0x0a, 0x1, 0x0a, 0x0a, 0x0a, 0x0a,
                /* type | code | checksum  |     id    |  seqnum   | */
                0x00,     0x00,  0xf0, 0x01, 0x00, 0x01, 0x00, 0x01
            );

            Net::Os::sleep(1);
            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;
            if(Net::getCounterStats().icmp.inputReceived != 3) return Task::bad;
            if(Net::getCounterStats().icmp.inputFormatError != 3) return Task::bad;
            if(Net::getCounterStats().icmp.outputQueued != 0) return Task::bad;
            if(Net::getCounterStats().icmp.outputSent != 0) return Task::bad;
            if(Net::getCounterStats().icmp.pingRequests != 0) return Task::bad;

            return ok;
        }
    } task;

    work(task);
}

TEST(NetIcmp, IcmpReceiveReplyNoListener) {
    struct Task: public TestTask<Task> {
        bool run() {
            auto initialRxUsage = Accessor::pool.statRxUsed();

            receiveReply();

        	Net::Os::sleep(1);

            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;
        	return ok;
        }
    } task;

    work(task);
}

TEST(NetIcmp, IcmpReceiveReplyWithListener) {
    struct Task: public TestTask<Task> {
        bool run() {
            auto initialRxUsage = Accessor::pool.statRxUsed();

            Net::IcmpReceiver r;

            r.init();
            r.receive();

            receiveReply();

			r.wait();

			if(!checkReplyContent(r))
				return Task::bad;

			r.close();

            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;
            if(Net::getCounterStats().icmp.inputReceived != 1) return Task::bad;
            if(Net::getCounterStats().icmp.inputProcessed != 1) return Task::bad;
            if(Net::getCounterStats().icmp.inputFormatError != 0) return Task::bad;
            if(Net::getCounterStats().icmp.outputQueued != 0) return Task::bad;
            if(Net::getCounterStats().icmp.outputSent != 0) return Task::bad;
            if(Net::getCounterStats().icmp.pingRequests != 0) return Task::bad;

        	return ok;
        }
    } task;

    work(task);
}

TEST(NetIcmp, IcmpReceiveMultipleRepliesWithListenerAtOnce) {
    struct Task: public TestTask<Task> {
        bool run() {
            auto initialRxUsage = Accessor::pool.statRxUsed();

            Net::IcmpReceiver r;

            r.init();
            r.receive();

            doIndirect([](){
                for(int i=0; i < 3; i++)
                    receiveReply();
            });

			for(int i=0; i < 3; i++) {
	            r.wait();

			    if(!checkReplyContent(r))
			        return Task::bad;
			}

			r.close();

            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;

            if(Net::getCounterStats().icmp.inputReceived != 3) return Task::bad;
            if(Net::getCounterStats().icmp.inputProcessed != 3) return Task::bad;
            if(Net::getCounterStats().icmp.inputFormatError != 0) return Task::bad;
            if(Net::getCounterStats().icmp.outputQueued != 0) return Task::bad;
            if(Net::getCounterStats().icmp.outputSent != 0) return Task::bad;
            if(Net::getCounterStats().icmp.pingRequests != 0) return Task::bad;

        	return ok;
        }
    } task;

    work(task);
}
