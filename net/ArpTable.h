/*
 * ArpTable.h
 *
 *  Created on: 2017.11.26.
 *      Author: tooma
 */

#ifndef ARPTABLE_H_
#define ARPTABLE_H_

#include "Network.h"

template<class S, class... Args>
struct Network<S, Args...>::ArpEntry {
    friend class Network<S, Args...>;
	AddressIp4 ip;
	AddressEthernet mac;
};

template<class S, class... Args>
template<size_t nEntries>
class Network<S, Args...>::ArpTable: SharedTable<Os, ArpEntry, nEntries>, Os::IoChannelBase<ArpTable<nEntries>> {
	inline ArpEntry* lookUp(AddressIp4 ip) {
		if(auto ret = find([ip](const ArpEntry* entry){return entry->ip == ip;})) {

		}

		return nullptr;
	}
};

#endif /* ARPTABLE_H_ */
