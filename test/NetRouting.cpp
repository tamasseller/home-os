/*
 * NetRouting.cpp
 *
 *  Created on: 2017.12.12.
 *      Author: tooma
 */

#include "common/CommonTestUtils.h"

#include "Routing.h"

using Os = OsRr;

static constexpr int interface1 = 1, interface2 = 2, interface3 = 3;


TEST_GROUP(NetRoutingTable) {
	typedef const int Interface;
	typedef RoutingTable<Os, Interface, 8> Uut;

	static constexpr Interface *i1 = &interface1, *i2 = &interface2, *i3 = &interface3;
};

TEST(NetRoutingTable, Simple) {
	struct Task: public TestTask<Task> {
		Uut uut;
		bool run() {
			if(!uut.add(Uut::Route(i1, AddressIp4::make(10, 0, 0, 0), 8), true)) return bad;
			auto x = uut.findRouteTo(AddressIp4::make(10, 10, 10, 10));
			if(!x || x->getDevice() != i1) return bad;

			if(!uut.remove(Uut::Route(i1, AddressIp4::make(10, 0, 0, 0), 8))) return bad;
			auto y = uut.findRouteTo(AddressIp4::make(10, 10, 10, 10));
			if(y) return bad;

			if(!uut.add(Uut::Route(i2, AddressIp4::make(3, 2, 1, 0), 13), true)) return bad;

			if(!x || x->getDevice() != i1) return bad;
			uut.releaseRoute(x);

			if(uut.remove(Uut::Route(i2, AddressIp4::make(1, 2, 3, 4), 17))) return bad;

			return ok;
		}
	} task;

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(NetRoutingTable, Overlapping) {
	struct Task: public TestTask<Task> {
		Uut uut;
		bool run() {
			if(!uut.add(Uut::Route(i1, AddressIp4::make(10, 10, 0, 1), 8, 10), true)) return bad;
			if(uut.add(Uut::Route(i1, AddressIp4::make(10, 10, 0, 1), 8, 20), true)) return bad;

			if(!uut.add(Uut::Route(i1, AddressIp4::make(10, 10, 10, 10), 8, 10), true)) return bad;
			if(uut.add(Uut::Route(i1, AddressIp4::make(10, 10, 0, 1), 8, 20), true)) return bad;

			if(!uut.add(Uut::Route(i2, AddressIp4::make(10, 10, 0, 1), 8), true)) return bad;
			if(uut.add(Uut::Route(i2, AddressIp4::make(10, 10, 0, 1), 8), true)) return bad;

			if(!uut.add(Uut::Route(i2, AddressIp4::make(10, 10, 0, 1), 16, 10), true)) return bad;
			if(uut.add(Uut::Route(i2, AddressIp4::make(10, 10, 0, 1), 16), true)) return bad;

			if(!uut.add(Uut::Route(i3, AddressIp4::make(10, 10, 10, 1), 24), true)) return bad;
			if(uut.add(Uut::Route(i3, AddressIp4::make(10, 10, 10, 1), 24), true)) return bad;

			/* Precedence:
			 * 		10.10.10.0/24 -> i3 metric 0
			 * 		10.10.0.0/16  -> i2 metric 10
			 * 		10.0.0.0/8    -> i1 metric 10
			 * 		10.0.0.0/8    -> i2 metric 0
			 */

			auto x = uut.findRouteTo(AddressIp4::make(10, 10, 10, 10));
			if(!x || !x->isDirect() || x->getDevice() != i3) return bad;	// first rule

			auto y = uut.findRouteTo(AddressIp4::make(10, 10, 20, 20));
			if(!y || !x->isDirect() || y->getDevice() != i2) return bad; // second rule

			auto z = uut.findRouteTo(AddressIp4::make(10, 11, 12, 13));
			if(!z || !x->isDirect() || z->getDevice() != i1) return bad; // third rule

			auto u = uut.findRouteTo(AddressIp4::make(1, 2, 3, 4));
			if(u) return bad; // no route

			uut.setDown(i1);

			auto v = uut.findRouteTo(AddressIp4::make(10, 11, 12, 13));
			if(!v || !v->isDirect() || v->getDevice() != i2) return bad; // fourth rule

			uut.setUp(i1);

			auto w = uut.findRouteTo(AddressIp4::make(10, 11, 12, 13));
			if(!w || !w->isDirect() || w->getDevice() != i1) return bad; // fourth rule

			return ok;
		}
	} task;

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(NetRoutingTable, Router) {
	struct Task: public TestTask<Task> {
		Uut uut;
		bool run() {
			// Some autoip stuff
			if(!uut.add(Uut::Route(i1, AddressIp4::make(169, 254, 0, 1), 16), true)) return bad;

			// LAN1
			if(!uut.add(Uut::Route(i2, AddressIp4::make(192, 168, 1, 7), 24), true)) return bad;

			// The Internet
			if(!uut.add(Uut::Route(i2, AddressIp4::make(192, 168, 1, 7), 0, AddressIp4::make(192, 168, 1, 1)), true))
				return bad;

			auto x = uut.findRouteTo(AddressIp4::make(169, 254, 1, 2));	// Autoip address
			if(!x || !x->isDirect() || x->getDevice() != i1) return bad;

			auto y = uut.findRouteTo(AddressIp4::make(192, 168, 1, 123));	// LAN address
			if(!y || !y->isDirect() || y->getDevice() != i2) return bad;

			auto z = uut.findRouteTo(AddressIp4::make(4, 4, 4, 4));		// Internet
			if(!z || z->isDirect() || z->getDevice() != i2) return bad;
			if(z->getGateway() != AddressIp4::make(192, 168, 1, 1)) return bad;

			return ok;
		}
	} task;

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(NetRoutingTable, Reception) {
	struct Task: public TestTask<Task> {
		Uut uut;
		bool run() {
			// Some autoip stuff
			if(!uut.add(Uut::Route(i1, AddressIp4::make(169, 254, 0, 1), 16), true)) return bad;

			// LAN1
			if(!uut.add(Uut::Route(i2, AddressIp4::make(192, 168, 1, 7), 24), true)) return bad;

			// The Internet
			if(!uut.add(Uut::Route(i2, AddressIp4::make(192, 168, 1, 7), 0, AddressIp4::make(192, 168, 1, 1)), true))
				return bad;


			auto a = uut.findRouteWithSource(i1, AddressIp4::make(169, 254, 0, 2));
			if(a) return bad;

			auto x = uut.findRouteWithSource(i1, AddressIp4::make(169, 254, 0, 1));
			if(!x || x->getDevice() != i1) return bad;

			auto y = uut.findRouteWithSource(i2, AddressIp4::make(192, 168, 1, 7));
			if(!y || y->getDevice() != i2) return bad;

			uut.setDown(i2);

			auto z = uut.findRouteWithSource(i2, AddressIp4::make(192, 168, 1, 123));
			if(z) return bad;

			return ok;
		}
	} task;

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

