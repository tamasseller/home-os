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

    static inline void receiveNoChecksum() {
        Net::template getEthernetInterface<DummyIf>()->receive(
            /*            dst                 |                src                | etherType */
            0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x08, 0x00,
            /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
            0x45, 0x00, 0x00, 0x22, 0x84, 0x65, 0x40, 0x00, 0x40, 0x11, 0x8e, 0x47,
            /* source IP address  | destination IP address */
            0x0a, 0x0a, 0x0a, 0x1, 0x0a, 0x0a, 0x0a, 0x0a,
            /* srcport|  dstport  | udplength | checksum */
            0x82, 0x3b, 0x04, 0xd2, 0x00, 0x0e, 0x00, 0x00,
            /* payload */
            'f', 'o', 'o', 'b', 'a', 'r'
        );
    }

    static inline void expectDurPur(int n=1) {
        Net::template getEthernetInterface<DummyIf>()->expectN(n,
            /*            dst                 |                src                | etherType */
            0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x08, 0x00,
            /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
            0x45, 0x00, 0x00, 0x38, 0x00, 0x00, 0x40, 0x00, 0xff, 0x1, 0x53, 0xa6,
            /* source IP address  | destination IP address */
            0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x1,
            /* typcod | checksum | rest of header */
            0x03, 0x03, 0xfc, 0xfc, 0x00, 0x00, 0x00, 0x00,

            /* Original datagram */
            0x45, 0x00, 0x00, 0x22, 0x84, 0x65, 0x40, 0x00, 0x40, 0x11, 0x8e, 0x47,
            /* source IP address  | destination IP address */
            0x0a, 0x0a, 0x0a, 0x1, 0x0a, 0x0a, 0x0a, 0x0a,
            /* srcport|  dstport  | udplength | checksum */
            0x82, 0x3b, 0x04, 0xd2, 0x00, 0x0e, 0x19, 0x62
        );
    }

    static inline void expectLong() {
    	Net::template getEthernetInterface<DummyIf>()->expectN(1,
    		/*            dst                 |                src                | etherType */
    		0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x08, 0x00,
    		/* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
    		0x45, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x40, 0x00, 0x40, 0x11, 0x12, 0x53,
    		/* source IP address  | destination IP address */
    		0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x1,
            /* srcport|  dstport  | udplength | checksum */
            0x82, 0x3b, 0x04, 0xd2, 0x00, 0x68, 0xdb, 0xae,
            /* payload */
            'f', 'o', 'o', 'b', 'a', 'r', 'f', 'o', 'o', 'b', 'a', 'r',
            'f', 'o', 'o', 'b', 'a', 'r', 'f', 'o', 'o', 'b', 'a', 'r',
            'f', 'o', 'o', 'b', 'a', 'r', 'f', 'o', 'o', 'b', 'a', 'r',
            'f', 'o', 'o', 'b', 'a', 'r', 'f', 'o', 'o', 'b', 'a', 'r',
            'f', 'o', 'o', 'b', 'a', 'r', 'f', 'o', 'o', 'b', 'a', 'r',
            'f', 'o', 'o', 'b', 'a', 'r', 'f', 'o', 'o', 'b', 'a', 'r',
            'f', 'o', 'o', 'b', 'a', 'r', 'f', 'o', 'o', 'b', 'a', 'r',
            'f', 'o', 'o', 'b', 'a', 'r', 'f', 'o', 'o', 'b', 'a', 'r'
    	);
    }

    static inline void receiveLong() {
    	Net::template getEthernetInterface<DummyIf>()->receive(
    		/*            dst                 |                src                | etherType */
			0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
    		/* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
    		0x45, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x40, 0x00, 0x40, 0x11, 0x12, 0x53,
    		/* source IP address  | destination IP address */
    		0x0a, 0x0a, 0x0a, 0x01, 0x0a, 0x0a, 0x0a, 0x0a,
            /* srcport|  dstport  | udplength | checksum */
    		0x04, 0xd2, 0x82, 0x3b, 0x00, 0x68, 0xdb, 0xae,
            /* payload */
            'f', 'o', 'o', 'b', 'a', 'r', 'f', 'o', 'o', 'b', 'a', 'r',
            'f', 'o', 'o', 'b', 'a', 'r', 'f', 'o', 'o', 'b', 'a', 'r',
            'f', 'o', 'o', 'b', 'a', 'r', 'f', 'o', 'o', 'b', 'a', 'r',
            'f', 'o', 'o', 'b', 'a', 'r', 'f', 'o', 'o', 'b', 'a', 'r',
            'f', 'o', 'o', 'b', 'a', 'r', 'f', 'o', 'o', 'b', 'a', 'r',
            'f', 'o', 'o', 'b', 'a', 'r', 'f', 'o', 'o', 'b', 'a', 'r',
            'f', 'o', 'o', 'b', 'a', 'r', 'f', 'o', 'o', 'b', 'a', 'r',
            'f', 'o', 'o', 'b', 'a', 'r', 'f', 'o', 'o', 'b', 'a', 'r'
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

            expectDurPur();
            receiveSimple();

            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;

            if(Net::getCounterStats().udp.inputReceived != 1) return Task::bad;
            if(Net::getCounterStats().udp.inputProcessed != 0) return Task::bad;
            if(Net::getCounterStats().udp.inputFormatError != 0) return Task::bad;
            if(Net::getCounterStats().udp.inputNoPort != 1) return Task::bad;
            if(Net::getCounterStats().udp.outputQueued != 0) return Task::bad;
            if(Net::getCounterStats().udp.outputSent != 0) return Task::bad;

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

            expectDurPur(2);

            doIndirect([](){
                for(int i=0; i<2; i++)
                    receiveSimple();
            });

            if(r.wait(1)) return Task::bad;

            r.close();

            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;

            if(Net::getCounterStats().udp.inputReceived != 2) return Task::bad;
            if(Net::getCounterStats().udp.inputProcessed != 0) return Task::bad;
            if(Net::getCounterStats().udp.inputFormatError != 0) return Task::bad;
            if(Net::getCounterStats().udp.inputNoPort != 2) return Task::bad;
            if(Net::getCounterStats().udp.outputQueued != 0) return Task::bad;
            if(Net::getCounterStats().udp.outputSent != 0) return Task::bad;


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
            if(r.getLength() != 6) return Task::bad;

            r.close();

            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;

            if(Net::getCounterStats().udp.inputReceived != 1) return Task::bad;
            if(Net::getCounterStats().udp.inputProcessed != 1) return Task::bad;
            if(Net::getCounterStats().udp.inputFormatError != 0) return Task::bad;
            if(Net::getCounterStats().udp.inputNoPort != 0) return Task::bad;
            if(Net::getCounterStats().udp.outputQueued != 0) return Task::bad;
            if(Net::getCounterStats().udp.outputSent != 0) return Task::bad;

            return ok;
        }
    } task;

    work(task);
}

TEST(NetUdp, ReceiveNoChecksum) {
    struct Task: public TestTask<Task> {
        bool run() {
            auto initialRxUsage = Accessor::pool.statRxUsed();

            Net::UdpReceiver r;

            r.init();
            r.receive(1234);

            receiveNoChecksum();

            r.wait();

            if(!checkContent(r, 'f', 'o', 'o', 'b', 'a', 'r')) return Task::bad;

            if(r.getPeerAddress() != AddressIp4::make(10, 10, 10, 1)) return Task::bad;
            if(r.getPeerPort() != 0x823b) return Task::bad;
            if(r.getLength() != 6) return Task::bad;

            r.close();

            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;

            if(Net::getCounterStats().udp.inputReceived != 1) return Task::bad;
            if(Net::getCounterStats().udp.inputProcessed != 1) return Task::bad;
            if(Net::getCounterStats().udp.inputFormatError != 0) return Task::bad;
            if(Net::getCounterStats().udp.inputNoPort != 0) return Task::bad;

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

            if(Net::getCounterStats().udp.inputReceived != 2) return Task::bad;
            if(Net::getCounterStats().udp.inputProcessed != 0) return Task::bad;
            if(Net::getCounterStats().udp.inputFormatError != 1) return Task::bad;
            if(Net::getCounterStats().udp.inputNoPort != 0) return Task::bad;

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

            if(Net::getCounterStats().udp.inputReceived != 3) return Task::bad;
            if(Net::getCounterStats().udp.inputProcessed != 0) return Task::bad;
            if(Net::getCounterStats().udp.inputFormatError != 2) return Task::bad;
            if(Net::getCounterStats().udp.inputNoPort != 0) return Task::bad;

            /*
             * Header truncated (consistent IP payload size)
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

            if(Net::getCounterStats().udp.inputReceived != 4) return Task::bad;
            if(Net::getCounterStats().udp.inputFormatError != 3) return Task::bad;

            /*
             * Header truncated (inconsistent IP payload size)
             */
            Net::template getEthernetInterface<DummyIf>()->receive(
                /*            dst                 |                src                | etherType */
                0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x08, 0x00,
                /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
                0x45, 0x00, 0x00, 0x1f, 0x84, 0x65, 0x40, 0x00, 0x40, 0x11, 0x8e, 0x4a,
                /* source IP address  | destination IP address */
                0x0a, 0x0a, 0x0a, 0x1, 0x0a, 0x0a, 0x0a, 0x0a,
                /* srcport|  dstport  | udplength | checksum */
                0x82, 0x3b, 0x04, 0xd2, 0x00, 0x0e, 0xf0
            );

            if(Net::getCounterStats().udp.inputReceived != 5) return Task::bad;
            if(Net::getCounterStats().udp.inputFormatError != 4) return Task::bad;

            /*
             * Header truncated (inconsistent IP payload size)
             */
            Net::template getEthernetInterface<DummyIf>()->receive(
                /*            dst                 |                src                | etherType */
                0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x08, 0x00,
                /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
                0x45, 0x00, 0x00, 0x1f, 0x84, 0x65, 0x40, 0x00, 0x40, 0x11, 0x8e, 0x4a,
                /* source IP address  | destination IP address */
                0x0a, 0x0a, 0x0a, 0x1, 0x0a, 0x0a, 0x0a, 0x0a,
                /* srcport|  dstport  | udplength | checksum */
                0x82, 0x3b, 0x04
            );

            if(Net::getCounterStats().udp.inputReceived != 6) return Task::bad;
            if(Net::getCounterStats().udp.inputFormatError != 5) return Task::bad;

            if(!r.wait(1)) return Task::bad;
            if(!checkContent(r, 'f', 'o', 'o', 'b', 'a', 'r')) return Task::bad;
            if(r.getPeerAddress() != AddressIp4::make(10, 10, 10, 1)) return Task::bad;
            if(r.getPeerPort() != 0x823b) return Task::bad;
            if(r.getLength() != 6) return Task::bad;

            if(Net::getCounterStats().udp.inputReceived != 6) return Task::bad;
            if(Net::getCounterStats().udp.inputProcessed != 1) return Task::bad;
            if(Net::getCounterStats().udp.inputFormatError != 5) return Task::bad;
            if(Net::getCounterStats().udp.inputNoPort != 0) return Task::bad;

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
                if(r.getLength() != 6) return Task::bad;
            }

            r.close();

            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;
            return ok;
        }
    } task;

    work(task);
}

TEST(NetUdp, SendLong) {
    struct Task: public TestTask<Task> {
        bool run() {
            typename Net::UdpTransmitter tx;
            tx.init(0x823b);

            if(!tx.prepare(AddressIp4::make(10, 10, 10, 1), 0x04d2, 6*16))
                return Task::bad;

            tx.wait();

            for(int i=0; i<16; i++)
				if(tx.fill("foobar", 6) != 6)
					return Task::bad;

            expectLong();

            if(!tx.send())
                return Task::bad;

            tx.wait();
            return Task::ok;
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

#if 1
TEST(NetUdp, Echo) {
    struct Task: public TestTask<Task> {

    	struct UdpEchoTask: public TestTask<UdpEchoTask> {
    		Net::UdpReceiver rx;
    		Net::UdpTransmitter tx;
    		Net::Os::BinarySemaphore sem;

    		bool run() {
    			rx.init();
    	        rx.receive(0x823b);
    	        tx.init(0x823b);

    	        for(int i = 0; i < 20; i++) {
    	            rx.wait();
    	            tx.prepare(rx.getPeerAddress(), rx.getPeerPort(), 128);

    	            if(tx.getError())
    	            	return bad;

    	            while(!rx.atEop()) {
    	                Net::Chunk c = rx.getChunk();
    	                rx.advance((uint16_t)c.length);
    					if(tx.fill(c.start, (uint16_t)c.length) != c.length)
    						break;
    	            }

    	            tx.send();

    	            if(tx.getError())
    	            	return bad;
    	        }

    	        tx.wait();

    	        rx.close();

    	        this->error = ok;
    	        sem.notifyFromTask();
    	        return ok;
    		}
    	} udpEchoTask;

        bool run() {
            typename Net::UdpTransmitter tx;

            udpEchoTask.sem.init(false);
            udpEchoTask.start();
            Net::Os::sleep(1);

			for(int i = 0; i < 10; i++) {
				expectLong();
				receiveLong();
				Net::Os::sleep(1);
			}

			for(int i = 0; i < 10; i++) {
				expectLong();
				receiveLong();
			}

			udpEchoTask.sem.wait();

			if(Net::getCounterStats().ip.outputRequest != Net::getCounterStats().ip.outputSent)
				return Task::bad;

            return udpEchoTask.error;
        }
    } task;

    work(task);
}
#endif
