/*
 * NetIcmp.cpp
 *
 *  Created on: 2017.12.14.
 *      Author: tooma
 */

#include "common/TestNetDefinitions.h"

using Net = Net64;
using Accessor = NetworkTestAccessor<Net>;

TEST_GROUP(NetIcmpArp)
{
	static inline void expectReply(uint16_t n) {
        Net::template getEthernetInterface<DummyIf>()->expectN(n,
            /*            dst                 |                src                | etherType */
			0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x08, 0x06,
            /* hwType | protoType |hSize|pSize|   opCode  |           sender MAC              */
            0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x02, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00,
            /*      sender IP     |          target MAC               |       target IP       */
            0x0a, 0x0a, 0x0a, 0x0a, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x0a, 0x0a, 0x0a, 0x01
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

TEST(NetIcmpArp, ArpRequestSimple) {
    struct Task: public TestTask<Task> {
        bool run() {
            auto initialRxUsage = Accessor::pool.statRxUsed();

        	expectReply(1);

        	doIndirect([](){
        		Net::template getEthernetInterface<DummyIf>()->receive(
					/*            dst                 |                src                | etherType */
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x06,
					/* hwType | protoType |hSize|pSize|   opCode  |           sender MAC              */
					0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x01, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e,
					/*      sender IP     |          target MAC               |       target IP       */
					0x0a, 0x0a, 0x0a, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x0a, 0x0a, 0x0a
        		);
        	});

        	Net::Os::sleep(1);

            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;
        	return ok;
        }
    } task;

    work(task);
}

TEST(NetIcmpArp, ArpRequestTriple) {
    struct Task: public TestTask<Task> {
        bool run() {
            auto initialRxUsage = Accessor::pool.statRxUsed();

        	expectReply(3);

        	doIndirect([](){
        		for(int i=0; i<3; i++) {
					Net::template getEthernetInterface<DummyIf>()->receive(
						/*            dst                 |                src                | etherType */
						0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x06,
						/* hwType | protoType |hSize|pSize|   opCode  |           sender MAC              */
						0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x01, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e,
						/*      sender IP     |          target MAC               |       target IP       */
						0x0a, 0x0a, 0x0a, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x0a, 0x0a, 0x0a
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

TEST(NetIcmpArp, ArpRequestOverflow) {
    struct Task: public TestTask<Task> {
        bool run() {
            auto initialRxUsage = Accessor::pool.statRxUsed();

        	doIndirect([](){
        		int16_t n = 0;

                Net::template getEthernetInterface<DummyIf>()->setDelayed(true);

				while(Net::template getEthernetInterface<DummyIf>()->receive(
					/*            dst                 |                src                | etherType */
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x06,
					/* hwType | protoType |hSize|pSize|   opCode  |           sender MAC              */
					0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x01, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e,
					/*      sender IP     |          target MAC               |       target IP       */
					0x0a, 0x0a, 0x0a, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x0a, 0x0a, 0x0a
				)) {
					n++;
				}

				expectReply(n);

                Net::template getEthernetInterface<DummyIf>()->setDelayed(false);
        	});

        	Net::Os::sleep(1);

        	expectReply(1);

			Net::template getEthernetInterface<DummyIf>()->receive(
				/*            dst                 |                src                | etherType */
				0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x06,
				/* hwType | protoType |hSize|pSize|   opCode  |           sender MAC              */
				0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x01, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e,
				/*      sender IP     |          target MAC               |       target IP       */
				0x0a, 0x0a, 0x0a, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x0a, 0x0a, 0x0a
			);

            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;
            return ok;
        }
    } task;

    work(task);
}

TEST(NetIcmpArp, ArpRequestWithJunk) {
    struct Task: public TestTask<Task> {
        bool run() {
            auto initialRxUsage = Accessor::pool.statRxUsed();

        	expectReply(1);

        	Net::template getEthernetInterface<DummyIf>()->receive(
				/*            dst                 |                src                | etherType */
				0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x06,
				/* hwType | protoType |hSize|pSize|   opCode  |           sender MAC              */
				0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x01, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e,
				/*      sender IP     |          target MAC               |       target IP       */
				0x0a, 0x0a, 0x0a, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x0a, 0x0a, 0x0a,
				/* 							           Garbage                                    */
				0x6a, 0x3b, 0xa6, 0xe0, 0x6a, 0x3b, 0xa6, 0xe0, 0x6a, 0x3b, 0xa6, 0xe0, 0x6a, 0x3b,
				/* 							           Garbage                                    */
				0xa6, 0xe0, 0x6a, 0x3b, 0xa6, 0xe0, 0x6a, 0x3b, 0xa6, 0xe0, 0x6a, 0x3b, 0xa6, 0xe0
        	);

        	Net::Os::sleep(1);

            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;
            return ok;
        }
    } task;

    work(task);
}

TEST(NetIcmpArp, ArpRequestNotForUs) {
    struct Task: public TestTask<Task> {
        bool run() {
            auto initialRxUsage = Accessor::pool.statRxUsed();

        	Net::template getEthernetInterface<DummyIf>()->receive(
				/*            dst                 |                src                | etherType */
				0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x06,
				/* hwType | protoType |hSize|pSize|   opCode  |           sender MAC              */
				0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x01, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e,
				/*      sender IP     |          target MAC               |       target IP       */
				0x0a, 0x0a, 0x0a, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x0b, 0x0c, 0x0d
        	);

        	Net::Os::sleep(1);

            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;
            return ok;
        }
    } task;

    work(task);
}

TEST(NetIcmpArp, ArpRequestInvalid) {
    struct Task: public TestTask<Task> {
        bool run() {
            auto initialRxUsage = Accessor::pool.statRxUsed();

			Net::template getEthernetInterface<DummyIf>()->receive(
				/*            dst                 |                src                | etherType */
				0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x06,
				/* hwType | protoType |hSize|pSize|   opCode  |           sender MAC              */
				0x1b, 0xad, 0xc0, 0xde, 0x06, 0x04, 0x00, 0x01, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e,
				/*      sender IP     |          target MAC               |       target IP       */
				0x0a, 0x0a, 0x0a, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x0a, 0x0a, 0x0a
			);

            Net::Os::sleep(1);
            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;

			Net::template getEthernetInterface<DummyIf>()->receive(
				/*            dst                 |                src                | etherType */
				0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x06,
				/* hwType | protoType |hSize|pSize|   opCode  |           sender MAC              */
				0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0xf0, 0x01, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e,
				/*      sender IP     |          target MAC               |       target IP       */
				0x0a, 0x0a, 0x0a, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x0a, 0x0a, 0x0a
			);

            Net::Os::sleep(1);
            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;

            Net::template getEthernetInterface<DummyIf>()->receive(
                  /*            dst                 |                src                | etherType */
                  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x06,
                  /* hwType | protoType |hSize|pSize|   opCode  |           sender MAC              */
                  0x00, 0x01, 0x08
            );

            Net::Os::sleep(1);
            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;

            Net::template getEthernetInterface<DummyIf>()->receive(
                  /*            dst                 |                src                | etherType */
                  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x06,
                  /* hwType | protoType |hSize|pSize|   opCode  |           sender MAC              */
                  0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00
            );

            Net::Os::sleep(1);
            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;

            Net::template getEthernetInterface<DummyIf>()->receive(
                  /*            dst                 |                src                | etherType */
                  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x06,
                  /* hwType | protoType |hSize|pSize|   opCode  |           sender MAC              */
                  0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x01
            );

            Net::Os::sleep(1);
            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;

            Net::template getEthernetInterface<DummyIf>()->receive(
                  /*            dst                 |                src                | etherType */
                  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x06,
                  /* hwType | protoType |hSize|pSize|   opCode  |           sender MAC              */
                  0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x01, 0x00, 0xac
            );

            Net::Os::sleep(1);
            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;

            Net::template getEthernetInterface<DummyIf>()->receive(
                  /*            dst                 |                src                | etherType */
                  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x06,
                  /* hwType | protoType |hSize|pSize|   opCode  |           sender MAC              */
                  0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x01, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e,
                  /*      sender IP     |          target MAC               |       target IP       */
                  0x0a
            );

            Net::Os::sleep(1);
            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;

            Net::template getEthernetInterface<DummyIf>()->receive(
                  /*            dst                 |                src                | etherType */
                  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x06,
                  /* hwType | protoType |hSize|pSize|   opCode  |           sender MAC              */
                  0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x01, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e,
                  /*      sender IP     |          target MAC               |       target IP       */
                  0x0a, 0x0a, 0x0a, 0x01, 0x00, 0x00
            );

            Net::Os::sleep(1);
            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;

            Net::template getEthernetInterface<DummyIf>()->receive(
                  /*            dst                 |                src                | etherType */
                  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e, 0x08, 0x06,
                  /* hwType | protoType |hSize|pSize|   opCode  |           sender MAC              */
                  0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x01, 0x00, 0xac, 0xce, 0x55, 0x1b, 0x1e,
                  /*      sender IP     |          target MAC               |       target IP       */
                  0x0a, 0x0a, 0x0a, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x0a, 0x0a
            );

            Net::Os::sleep(1);
            if(Accessor::pool.statTxUsed()) return Task::bad;
            if(Accessor::pool.statRxUsed() != initialRxUsage) return Task::bad;

            return ok;
        }
    } task;

    work(task);
}
