/*
 * Routing.h
 *
 *  Created on: 2017.11.21.
 *      Author: tooma
 */

#ifndef ROUTING_H_
#define ROUTING_H_

#include "AddressIp4.h"
#include "SharedTable.h"

template<class Interface>
class Route {
	template<class, class, size_t> friend class RoutingTable;

	Interface* dev;
	AddressIp4 src, dst, via;
	uint8_t mask;
	uint8_t metric;
	bool up = false;

public:
	/// Create an **direct** route (directly reachable, no intermediate router).
	inline Route(Interface* dev, AddressIp4 addr, uint8_t mask, uint8_t metric = 0):
	    dev(dev), src(addr), dst(addr), via(AddressIp4::allZero), mask(mask), metric(metric) {}

	/// Create a route with default source address.
	inline Route(Interface* dev, AddressIp4 addr, uint8_t mask, AddressIp4 gateway, uint8_t metric = 0):
	    dev(dev), src(addr), dst(addr), via(gateway), mask(mask), metric(metric) {}

	/// Create a route with non-default source address.
	inline Route(Interface* dev, AddressIp4 dst, uint8_t mask, AddressIp4 gateway, AddressIp4 src, uint8_t metric = 0):
	    dev(dev), src(src), dst(dst), via(gateway), mask(mask), metric(metric) {}

	/**
	 * Get the source address to be used for the route.
	 *
	 * The returned unicast address is needed to be used as the source address for
	 * packets sent through this route.
	 */
	inline const AddressIp4 &getSource() {
		return src;
	}

	/**
	 * Is the destination reachable directly?
	 *
	 * Returns true if the destination is reachable without going through a router,
	 * this information is used for assigning the appropriate L2 address to a packet.
	 */
	inline bool isDirect() {
		return via == AddressIp4::allZero;
	}

	/**
	 * Get the gateway of an indirect route.
	 *
	 * If the destination is reachable through a gateway returns the address of it,
	 * AddressIp4::allZero (address 0.0.0.0) otherwise.
	 */
	inline const AddressIp4 &getGateway() {
		return via;
	}

	/**
	 * Get the interface the route goes through.
	 */
	inline Interface *getDevice() {
		return dev;
	}

	/**
	 * Default comparison operator.
	 *
	 * Returns true for routes that can not coexist in the routing table.
	 */
	inline bool operator ==(const Route &other) const {
		return dev == other.dev && dst == other.dst && mask == other.mask;
	}
};

template<class Os, class Interface, size_t routingTableEntries>
class RoutingTable: SharedTable<Os, Route<Interface>, routingTableEntries>
{
public:
	using Route = ::Route<Interface>;

private:
	inline void setUpDown(Interface* interface, bool isUp) {
	    for(size_t base = 0; auto x = this->find([=](const Route* r){return r->dev == interface;}, base);) {
	    	x->access().up = isUp;
	    	x->release();
	    }
	}

public:

	/**
	 * Add an entry to the routing table.
	 *
	 * @param route The route to add.
	 * @param setUp Specifies wether the route should be set 'up' initially.
	 */
	inline bool add(const Route& route, bool setUp = false) {
	    auto entry = this->findOrAllocate([&route](Route* other){
	        return route == *other;
	    });

	    if(entry->isValid()) {
	        entry->release();
	        return false;
	    }

	    entry->access() = route;
	    entry->access().up = setUp;
	    entry->finalize();
		return true;
	}

	/**
	 * Remove an entry from the routing table.
	 *
	 * @param route The route to remove.
	 */
	inline bool remove(const Route& route) {
	    auto entry = this->find([&route](Route* other){
	        return route == *other;
	    });

	    if(entry && entry->isValid()) {
	    	entry->access().up = false;
	        entry->erase();
	        return true;
	    }

		return false;
	}

	/**
	 * Set 'up' all of the routes through the specified interface.
	 */
	inline void setUp(Interface* interface) {
		setUpDown(interface, true);
	}

	/**
	 * Set 'down' all of the routes through the specified interface.
	 */
	inline void setDown(Interface* interface) {
		setUpDown(interface, false);
	}

	/**
	 * Find suitable route to a specific destination.
	 */
	inline Route* findRouteTo(const AddressIp4& dst)
	{
	    auto entry = this->findBest([&dst](Route* route){
            return (route->up &&										// If route is enabled (ie. link is up)
            		(route->dst / route->mask == dst / route->mask)) ?	// If sub-net matches
            		((route->mask << 8) | route->metric) + 1:			// Plus 1 for default 0 metric route case.
            		0;													// Zero never matches
        });

	    if(!entry)
	        return nullptr;

	    return &entry->access();
	}

	/**
	 * Find route with the specific source address and interface.
	 */
	inline Route* findRouteWithSource(Interface* interface, const AddressIp4& src)
	{

	    auto entry = this->findBest([&src, &interface](Route* route){
            return (route->up &&										// If route is enabled (ie. link is up)
            		(route->dev == interface) &&						// If device matches
            		(route->src == src)) ?								// If address matches
            		route->metric + 1:									// Plus 1 for 0 metric route case.
            		0;													// Zero never matches
        });

	    if(!entry)
	        return nullptr;

	    return &entry->access();
	}


	inline void releaseRoute(Route* route) {
		this->entryFromData(route)->release();
	}
};

#endif /* ROUTING_H_ */
