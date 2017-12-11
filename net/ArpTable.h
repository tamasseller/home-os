/*
 * ArpTable.h
 *
 *  Created on: 2017.11.26.
 *      Author: tooma
 */

#ifndef ARPTABLE_H_
#define ARPTABLE_H_

#include "AddressIp4.h"
#include "SharedTable.h"
#include "AddressEthernet.h"

struct ArpEntry {
	AddressIp4 ip;
	AddressEthernet mac;
	uint16_t timeout;
};

template<class Os>
class ArpTableIoData: public Os::IoJob::Data {
	friend class pet::LinkedList<ArpTableIoData>;
	ArpTableIoData* next;
public:
	union {
		AddressIp4 ip;
		AddressEthernet mac;
	};
};

template<class Os, size_t nEntries, class Base = typename Os::IoChannel>
class ArpTable: SharedTable<Os, ArpEntry, nEntries>, public Os::template IoChannelBase<ArpTable<Os, nEntries, Base>, Base>
{
	friend class ArpTable::IoChannelBase;

public:
	typedef ArpTableIoData<Os> Data;

private:

	/// List of items added recently
	pet::LinkedList<Data> newItems;

	/// List of items added before the last ageing.
	pet::LinkedList<Data> agedItems;

	/// The current time by which the entries are timed out.
	uint16_t ageingCounter = 0;

	/// Implementation of the IoChannelBase interface.
	inline bool hasJob() {
		return true;
	}

	/// Implementation of the IoChannelBase interface.
	inline bool addItem(typename Os::IoJob::Data* job)
	{
		Data* data = static_cast<Data*>(job);

		if(lookUp(data->ip, data->mac)) {
			this->jobDone(data);
			return true;
		}

		return newItems.addBack(data);
	}

	/// Implementation of the IoChannelBase interface.
	inline bool removeCanceled(typename Os::IoJob::Data* job) {
		auto data = static_cast<Data*>(job);

		if(!newItems.remove(data))
			agedItems.remove(data);

		return true;
	}

	inline bool flushRequests(typename pet::LinkedList<Data>::Iterator it, AddressIp4 ip, const AddressEthernet& mac)
	{
		bool ret = false;

		for(; it.current(); it.step()) {
			if(it.current()->ip == ip) {
				auto x = it.current();
				it.remove();
				x->mac = mac;
				this->jobDone(x);
				ret = true;
			}
		}

		return ret;
	}

public:
	/**
	 * Try to resolve an address immediately.
	 *
	 * @return True on success.
	 */
	inline bool lookUp(AddressIp4 ip, AddressEthernet& mac) {
		if(auto x = this->find([ip](const ArpEntry* entry){return entry->ip == ip;})) {
			mac = x->access().mac;
			return true;
		}

		return false;
	}

	/**
	 * Add an ARP entry manually.
	 */
	inline void set(AddressIp4 ip, const AddressEthernet &mac, uint16_t timeout) {
		while(auto ret = this->findOrAllocate([ip](const ArpEntry* entry){return entry->ip == ip;})) {
			if(ret->isValid()) {
				ret->erase();
				continue;
			}

			ret->access().ip = ip;
			ret->access().mac = mac;
			ret->access().timeout = timeout;
			ret->finalize();
			break;
		}
	}

	/**
	 * Add an entry received from external communication.
	 *
	 * @NOTE Allowed to be called from event handler context only!
	 */
	inline void handleResolved(AddressIp4 ip, const AddressEthernet &mac, uint16_t timeout) {
		this->set(ip, mac, timeout);
		if(!flushRequests(newItems.iterator(), ip, mac)) {
			flushRequests(agedItems.iterator(), ip, mac);
		}
	}

	/**
	 * Handle ageing of stored data and outstanding requests.
	 *
	 * @NOTE Allowed to be called from event handler context only!
	 */
	inline void ageContent() {
		this->ageingCounter++;
		while(auto *x = this->find([this](const ArpEntry* e){
			return (static_cast<int16_t>(ageingCounter - e->timeout) > 0);
		})) {
            x->erase();
        }

		for(auto it = agedItems.iterator(); auto x = it.current(); it.step()) {
			x->mac = AddressEthernet::allZero;
			this->jobDone(x);
		}

		agedItems.take(newItems);
	}

	/**
	 * Remove an ARP entry manually.
	 */
	inline void kill(AddressIp4 ip) {
		while(auto ret = this->find([ip](const ArpEntry* entry){return entry->ip == ip;})) {
			ret->erase();
		}
	}
};

#endif /* ARPTABLE_H_ */
