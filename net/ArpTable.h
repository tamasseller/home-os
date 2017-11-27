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
	uintptr_t timeout;
};

template<class S, class... Args>
class Network<S, Args...>::ArpTableIoData: public Os::IoJob::Data {
	friend class pet::LinkedList<ArpTableIoData>;
	ArpTableIoData* next;
public:
	union {
		AddressIp4 ip;
		ArpEntry* entry;
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

		if(auto entry = lookUp(data->ip)) {
			data->entry = entry;
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
	inline ArpEntry* lookUp(AddressIp4 ip) {
		if(auto x = this->find([ip](const ArpEntry* entry){return entry->ip == ip;}))
			return x->getData();

		return nullptr;
	}

	inline void free(ArpEntry* entry) {
		ArpTable::SharedTable::entryFromData(entry)->release();
	}

	inline void set(AddressIp4 ip, const AddressEthernet &mac, uintptr_t timeout) {
		if(auto ret = this->findOrAllocate([ip](const ArpEntry* entry){return entry->ip == ip;})) {
			if(ret->isValid()) {
				ret->release();
				return;
			}

			ret->getData()->timeout = timeout;
			ret->getData()->ip = ip;
			ret->getData()->mac = mac;
			ret->finalize();
		}

		for(auto it = this->items.iterator(); it.current(); it.step()) {
			if(it.current()->ip == ip) {
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
