/*
 * NetIpTransmission.cpp
 *
 *  Created on: 2017.11.19.
 *      Author: tooma
 */

#include "common/TestNetDefinitions.h"

TEST_GROUP(NetIpTransmission) {
	static void expectArpReq(size_t n) {
		DummyIf<0>::expectN(n,
			/*            dst                 |                src                | etherType */
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x08, 0x06,
			/* hwType | protoType |hSize|pSize|   opCode  |           sender MAC              */
			0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x01, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00,
			/*      sender IP     |          target MAC               |       target IP       */
			0x0a, 0x0a, 0x0a, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x0a, 0x0a, 0x01
		);
	}

	static void expectIp() {
		DummyIf<0>::expectN(1,
			/*            dst                 |                src                | etherType */
			0x00, 0xAC, 0xCE, 0x55, 0x1B, 0x1E, 0xee, 0xee, 0xee, 0xee, 0xee, 0x00, 0x08, 0x00
			/* TBD */
		);
	}


	template<bool addRoute, bool addArp, class Task>
	void work(Task& task)
	{
		task.start();
		Net::init(NetBuffers<Net>::buffers);

		if(addRoute) {
			Net::addRoute(Net::Route(
					AddressIp4::make(10, 10, 10, 0), 8,
					AddressIp4::make(10, 10, 10, 10), Net::getIf<DummyIf<0>>())
			);
		}

		if(addArp) {
			Net::getIf<DummyIf<0>>()->getArpCache()->set(
					AddressIp4::make(10, 10, 10, 1),
					AddressEthernet::make(0x00, 0xAC, 0xCE, 0x55, 0x1B, 0x1E),
					32767
			);
		}

		CommonTestUtils::start();
		CHECK(!task.error);
	}
};

TEST(NetIpTransmission, NoRoute) {
	struct Task: public TestTask<Task> {
		bool run() {
			Net::IpTransmission tx;
			tx.init();
			if(tx.prepare(AddressIp4::make(10, 10, 10, 1), 6))
				return bad;

			if(tx.getError() != NetErrorStrings::noRoute)
				return bad;

			return ok;
		}
	} task;

	work<false, false>(task);
}


TEST(NetIpTransmission, Unresolved) {
	struct Task: public TestTask<Task> {
		bool run() {
			Net::IpTransmission tx;
			tx.init();

			expectArpReq(4);

			if(!tx.prepare(AddressIp4::make(10, 10, 10, 1), 6))
				return bad;

			tx.wait();

			if(tx.getError() != NetErrorStrings::unresolved)
				return bad;

			tx.fill("foobar", 6);
			tx.send(254);
			return ok;
		}
	} task;

	work<true, false>(task);
}

TEST(NetIpTransmission, Resolved) {
	struct Task: public TestTask<Task> {
		bool run() {
			Net::IpTransmission tx;
			tx.init();

			expectArpReq(1);

			if(!tx.prepare(AddressIp4::make(10, 10, 10, 1), 6))
				return bad;

			if(!tx.shouldWait())
				return bad;

			Net::getIf<DummyIf<0>>()->getArpCache()->set(
					AddressIp4::make(10, 10, 10, 1),
					AddressEthernet::make(0x00, 0xAC, 0xCE, 0x55, 0x1B, 0x1E),
					32768
			);

			tx.wait();

			if(tx.getError())
				return bad;

/*			tx.fill("foobar", 6);
			tx.send(254);*/
			return ok;
		}
	} task;

	work<true, false>(task);
}


TEST(NetIpTransmission, Successful) {
	struct Task: public TestTask<Task> {
		bool run() {
			Net::IpTransmission tx;
			tx.init();
			if(!tx.prepare(AddressIp4::make(10, 10, 10, 1), 6))
				return bad;

			if(tx.shouldWait())
				return bad;

			if(tx.getError())
				return bad;

/*			tx.fill("foobar", 6);
			tx.send(254);*/
			return ok;
		}
	} task;

	work<true, true>(task);
}
