/*
 * NetTcp.cpp
 *
 *  Created on: 2018.01.14.
 *      Author: tooma
 */

#include "common/TestNetDefinitions.h"

using Net = Net64;
using Accessor = NetworkTestAccessor<Net>;

TEST_GROUP(NetTcp)
{
/*    template<class... C>
    static bool checkContent(Net::TcpReceiver &r, C... c) {
        const uint8_t expected[] = {static_cast<uint8_t>(c)...};

        for(uint8_t e: expected) {
            uint8_t a;
            if(!r.read8(a) || a != e)
                return false;
        }

        return true;
    }*/

    static inline void receiveSyn() {
        Net::template getEthernetInterface<DummyIf>()->receive(
            /*            dst                 |                src                | etherType */
			0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x00,
            /* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
            0x45, 0x00, 0x00, 0x2e, 0x84, 0x65, 0x40, 0x00, 0x40, 0x06, 0x8e, 0x46,
            /* source IP address  | destination IP address */
            0x0a, 0x0a, 0x0a, 0x1, 0x0a, 0x0a, 0x0a, 0x0a,
            /* srcport|  dstport  |    Sequence number    */
            0x12, 0x34, 0x56, 0x78, 0x00, 0x00, 0x00, 0x00,
    		/*     Ack number     | off+flags | wnd size */
    		0x00, 0x00, 0x00, 0x00, 0x50, 0x02, 0x01, 0x00,
    		/* checksum / urgent bs */
    		0xe6, 0xcd, 0x00, 0x00,
            /* payload */
            'f', 'o', 'o', 'b', 'a', 'r'
        );
    }

    static inline void expectRst() {
    	Net::template getEthernetInterface<DummyIf>()->expectN(1,
			/*            dst                 |                src                | etherType */
			0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x08, 0x00,
			/* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
			0x45, 0x00, 0x00, 0x28, 0x00, 0x00, 0x40, 0x00, 0xff, 0x06, 0x53, 0xb1,
			/* source IP address  | destination IP address */
			0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x1,
			/* srcport|  dstport  |    Sequence number    */
			0x56, 0x78, 0x12, 0x34, 0x00, 0x00, 0x00, 0x00,
			/*     Ack number     | off+flags | wnd size */
			0x00, 0x00, 0x00, 0x07, 0x50, 0x14, 0x00, 0x00,
			/* checksum / urgent bs */
			0x1e, 0xff, 0x00, 0x00
    	);
    }

    static inline void expectSynAck() {
    	Net::template getEthernetInterface<DummyIf>()->expectN(1,
			/*            dst                 |                src                | etherType */
			0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x08, 0x00,
			/* bullsh |  length   | frag. id  | flags+off | TTL |proto|  checksum */
			0x45, 0x00, 0x00, 0x28, 0x00, 0x00, 0x40, 0x00, 0xff, 0x06, 0x53, 0xb1,
			/* source IP address  | destination IP address */
			0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x1,
			/* srcport|  dstport  |    Sequence number    */
			0x56, 0x78, 0x12, 0x34, 0x00, 0x00, 0x00, 0x00,
			/*     Ack number     | off+flags | wnd size */
			0x00, 0x00, 0x00, 0x01, 0x05, 0x12, 0x00, 0x00,
			/* checksum / urgent bs */
			0x1f, 0x07, 0x00, 0x00
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

TEST(NetTcp, DropRawNoListenerAtAll) {
    struct Task: public TestTask<Task> {
        bool run() {
            auto initialRxUsage = Accessor::pool.statRxUsed();

            expectRst();
            receiveSyn();

            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;

            if(Net::getCounterStats().tcp.inputReceived != 1) return Task::bad;
            if(Net::getCounterStats().tcp.inputProcessed != 0) return Task::bad;
            if(Net::getCounterStats().tcp.inputFormatError != 0) return Task::bad;
            if(Net::getCounterStats().tcp.inputNoPort != 1) return Task::bad;
            if(Net::getCounterStats().tcp.inputConnectionDenied != 0) return Task::bad;
            if(Net::getCounterStats().tcp.outputQueued != 1) return Task::bad;
            if(Net::getCounterStats().tcp.outputSent != 1) return Task::bad;

            return ok;
        }
    } task;

    work(task);
}

TEST(NetTcp, DropNoMatchingListener) {
    struct Task: public TestTask<Task> {
        bool run() {
            auto initialRxUsage = Accessor::pool.statRxUsed();

            Net::TcpListener r;

            r.init();
            r.listen(0xf001);

            expectRst();
            receiveSyn();

            if(r.wait(1)) return Task::bad;

            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;

            if(Net::getCounterStats().tcp.inputReceived != 1) return Task::bad;
            if(Net::getCounterStats().tcp.inputProcessed != 0) return Task::bad;
            if(Net::getCounterStats().tcp.inputFormatError != 0) return Task::bad;
            if(Net::getCounterStats().tcp.inputNoPort != 1) return Task::bad;
            if(Net::getCounterStats().tcp.inputConnectionDenied != 0) return Task::bad;
            if(Net::getCounterStats().tcp.outputQueued != 1) return Task::bad;
            if(Net::getCounterStats().tcp.outputSent != 1) return Task::bad;

            r.close();

            return ok;
        }
    } task;

    work(task);
}

TEST(NetTcp, DenyConnection) {
    struct Task: public TestTask<Task> {
        bool run() {
            auto initialRxUsage = Accessor::pool.statRxUsed();

            Net::TcpListener r;

            r.init();
            r.listen(0x5678);

            receiveSyn();

            if(!r.wait(1)) return Task::bad;
            if(r.getPeerPort() != 0x1234) return Task::bad;
            if(r.getPeerAddress() != AddressIp4::make(10, 10, 10, 1)) return Task::bad;

            expectRst();

            r.deny();

            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;

            if(Net::getCounterStats().tcp.inputReceived != 1) return Task::bad;
            if(Net::getCounterStats().tcp.inputProcessed != 1) return Task::bad;
            if(Net::getCounterStats().tcp.inputFormatError != 0) return Task::bad;
            if(Net::getCounterStats().tcp.inputNoPort != 0) return Task::bad;
            if(Net::getCounterStats().tcp.inputConnectionDenied != 1) return Task::bad;
            if(Net::getCounterStats().tcp.outputQueued != 1) return Task::bad;
            if(Net::getCounterStats().tcp.outputSent != 1) return Task::bad;

            r.close();

            return ok;
        }
    } task;

    work(task);
}

#if 1
TEST(NetTcp, AcceptConnection) {
    struct Task: public TestTask<Task> {
        bool run() {
            auto initialRxUsage = Accessor::pool.statRxUsed();

            Net::TcpListener r;

            r.init();
            r.listen(0x5678);

            receiveSyn();

            if(!r.wait(1)) return Task::bad;
            if(r.getPeerPort() != 0x1234) return Task::bad;
            if(r.getPeerAddress() != AddressIp4::make(10, 10, 10, 1)) return Task::bad;

            expectSynAck();

            Net::TcpSocket s;

            if(!r.accept(s)) return Task::bad;

            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;

            if(Net::getCounterStats().tcp.inputReceived != 1) return Task::bad;
            if(Net::getCounterStats().tcp.inputProcessed != 1) return Task::bad;
            if(Net::getCounterStats().tcp.inputFormatError != 0) return Task::bad;
            if(Net::getCounterStats().tcp.inputNoPort != 0) return Task::bad;
            if(Net::getCounterStats().tcp.inputConnectionDenied != 1) return Task::bad;
            if(Net::getCounterStats().tcp.outputQueued != 1) return Task::bad;
            if(Net::getCounterStats().tcp.outputSent != 1) return Task::bad;

            r.close();

            return ok;
        }
    } task;

    work(task);
}
#endif
