/*
 * Interfaces.h
 *
 *  Created on: 2017.11.21.
 *      Author: tooma
 */

#ifndef INTERFACES_H_
#define INTERFACES_H_

#include "Network.h"

#include "meta/ApplyToPack.h"

template<class S, class... Args>
class Network<S, Args...>::Interface {
	friend class Network<S, Args...>;

	virtual typename Os::IoChannel *getSender() = 0;

	virtual const AddressEthernet& getAddress() = 0;

	virtual bool requestResolution(typename Os::IoJob::Hook, typename Os::IoJob*, typename Os::IoJob::Callback, ArpTableIoData*) = 0;
	virtual ArpEntry* resolveAddress(const AddressIp4&) = 0;
	virtual void releaseAddress(ArpEntry*) = 0;
};

template<class S, class... Args>
template<class If>
class Network<S, Args...>::TxBinder: public Interface {
	friend class Network<S, Args...>;

	class Sender;
	Sender sender;
	ArpTable<If::arpCacheEntries> resolver;

	virtual typename Os::IoChannel *getSender() override final {
		return &sender;
	}

	virtual const AddressEthernet& getAddress() {
		return If::ethernetAddress;
	}

	virtual ArpEntry* resolveAddress(const AddressIp4& ip) override final {
		return resolver.lookUp(ip);
	}

	virtual bool requestResolution(typename Os::IoJob::Hook hook, typename Os::IoJob* item, typename Os::IoJob::Callback callback, ArpTableIoData* data) override final {
		return item->submitTimeout(hook, &resolver, If::arpReqTimeout, callback, data);
	}

	virtual void releaseAddress(ArpEntry* entry) override final {
		resolver.free(entry);
	}

	void init() {
		sender.init();
		resolver.init();
		If::init();
	}

public:
	Sender *getTxInfoProvider() {
		return &sender;
	}

	ArpTable<If::arpCacheEntries> *getArpCache() {
		return &resolver;
	}
};

template<class S, class... Args>
template<class... Input>
class Network<S, Args...>::Ifs<typename NetworkOptions::Set<Input...>, void>: public TxBinder<Input>... {
    typedef void (*link)(Ifs* const ifs);
    static void nop(Ifs* const ifs) {}

    template<link rest, class C, class...>
	struct Initializer {
		static inline void init(Ifs* const ifs) {
			static_cast<Network<S, Args...>::TxBinder<C>*>(ifs)->init();
			rest(ifs);
		}

		static constexpr link value = &init;
	};

public:
	void init() {
        (pet::ApplyToPack<link, Initializer, &Ifs::nop, Input...>::value)(this);
	}
};

template<class S, class... Args>
template<class C>
constexpr inline typename Network<S, Args...>::template TxBinder<C> *Network<S, Args...>::getIf() {
	return static_cast<TxBinder<C>*>(&state.interfaces);
}


template<class S, class... Args>
template<class If>
class Network<S, Args...>::TxBinder<If>::Sender: Network<S, Args...>::Os::template IoChannelBase<Sender> {
	friend class Sender::IoChannelBase;
	friend class TxBinder;

	TxPacket *currentPacket, *nextPacket;
	pet::LinkedList<TxPacket> items;

	void enableProcess() {If::enableTxIrq();}
	void disableProcess() {If::disableTxIrq();}

	bool addItem(typename Os::IoJob::Data* data)
	{
		TxPacket* p = static_cast<TxPacket*>(data);

		if(!currentPacket)
			currentPacket = p;
		else if(!nextPacket)
			nextPacket = p;
		else
			return this->items.addBack(p);

		return true;
	}

	bool removeItem(typename Os::IoJob::Data* data) {
		Os::assert(data == currentPacket, "Invalid network packet queue state");
		currentPacket = nextPacket;
		auto it = items.iterator();

		if((nextPacket = it.current()) != nullptr)
			it.remove();

		return true;
	}

	bool removeCanceled(typename Os::IoJob::Data* data) {
		return this->items.remove(static_cast<TxPacket*>(data));
	}

	bool hasJob() {
		return currentPacket != nullptr;
	}

public:
	inline TxPacket* getCurrentPacket() {
		return currentPacket;
	}

	inline TxPacket* getNextPacket() {
		return nextPacket;
	}

	void packetTransmitted() {
		this->jobDone(currentPacket);
	}
};

#endif /* INTERFACES_H_ */
