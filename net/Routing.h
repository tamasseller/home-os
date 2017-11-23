/*
 * Routing.h
 *
 *  Created on: 2017.11.21.
 *      Author: tooma
 */

#ifndef ROUTING_H_
#define ROUTING_H_

#include "Network.h"

template<class S, class... Args>
class Network<S, Args...>::Route {
    friend class Network<S, Args...>;
	AddressIp4 net;
	AddressIp4 src;
	Interface* dev;
	uint8_t mask;
	uint8_t metric;

public:
	inline Route(AddressIp4 net, uint8_t mask, AddressIp4 src, Interface* dev, uint8_t metric = 0):
	    net(net), src(src), dev(dev), mask(mask), metric(metric) {}
};

template<class S, class... Args>
class Network<S, Args...>::RoutingTable: SharedTable<Os, Route, routingTableEntries> {
public:
	bool add(const Route& route) {
	    auto entry = this->findOrAllocate([&route](Route* other){
	        return route.net == other->net && route.src == other->src && route.dev == other->dev && route.mask == other->mask;
	    });

	    if(entry->isValid()) {
	        entry->release();
	        return false;
	    }

	    *entry->getData() = route;
	    entry->finalize();
		return true;
	}

	Route* findRoute(const AddressIp4& dst)
	{
	    auto entry = this->findBest([&dst](Route* route){
            return (route->net / route->mask != dst / route->mask) ? 0 : (route->mask << 8) | route->metric;
        });

	    if(!entry)
	        return nullptr;

	    return entry->getData();
	}
};



#endif /* ROUTING_H_ */
