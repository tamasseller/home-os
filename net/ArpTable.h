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
	uint16_t timeout;
};

template<class S, class... Args>
class Network<S, Args...>::ArpTableIoData: public Os::IoJob::Data {
	friend class pet::LinkedList<ArpTableIoData>;
	ArpTableIoData* next;
public:
	union {
		AddressIp4 ip;
		AddressEthernet mac;
	};
};


template<class S, class... Args>
template<size_t nEntries>
class Network<S, Args...>::ArpTable:
	SharedTable<Os, ArpEntry, nEntries>,
	public Os::template IoChannelBase<ArpTable<nEntries>>
{
	friend class ArpTable::IoChannelBase;

	pet::LinkedList<ArpTableIoData> items;

	inline bool hasJob() {
		return true;
	}

	inline bool addItem(typename Os::IoJob::Data* job)
	{
		ArpTableIoData* data = static_cast<ArpTableIoData*>(job);

		if(lookUp(data->ip, data->mac)) {
			this->jobDone(data);
			return true;
		}

		return items.addBack(data);
	}

	inline bool removeItem(typename Os::IoJob::Data* job) {
		items.remove(static_cast<ArpTableIoData*>(job));
		return true;
	}

public:
	inline bool lookUp(AddressIp4 ip, AddressEthernet& mac) {
		if(auto x = this->find([ip](const ArpEntry* entry){return entry->ip == ip;})) {
			mac = x->access().mac;
			return true;
		}

		return false;
	}

	inline void set(AddressIp4 ip, const AddressEthernet &mac, uint16_t timeout) {
		if(auto ret = this->findOrAllocate([ip](const ArpEntry* entry){return entry->ip == ip;})) {
			if(ret->isValid()) {
				ret->release();
				return;
			}

			ret->access().ip = ip;
			ret->access().mac = mac;
			ret->access().timeout = timeout;
			ret->finalize();
		}

		for(auto it = this->items.iterator(); it.current(); it.step()) {
			if(it.current()->ip == ip) {
				it.current()->mac = mac;
				this->jobDone(it.current());
			}
		}
	}

	inline void kill(AddressIp4 ip) {
		while(auto ret = this->find([ip](const ArpEntry* entry){return entry->ip == ip;})) {
			ret->erase();
		}
	}
};

#endif /* ARPTABLE_H_ */
