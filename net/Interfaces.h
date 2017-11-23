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
class Network<S, Args...>::Interface {
	friend class Network<S, Args...>;
	virtual typename Os::IoChannel *getAllocator() = 0;
	virtual typename Os::IoChannel *getSender() = 0;
	virtual typename Packet::Operations const *getStandardPacketOperations() = 0;
};

template<class S, class... Args>
template<class If>
class Network<S, Args...>::TxBinder: Interface {
	friend class Network<S, Args...>;

	class Sender: Network<S, Args...>::Os::template IoChannelBase<Sender> {
		friend class Sender::IoChannelBase;
		friend class TxBinder;

		pet::LinkedList<TxPacket> items;

		void enableProcess() {If::enableTxIrq();}
		void disableProcess() {If::disableTxIrq();}
		bool addItem(typename Os::IoJob::Data* data) {
			return this->items.addBack(static_cast<TxPacket*>(data));
			return false;
		}

		bool removeItem(typename Os::IoJob::Data* data) {
			return this->items.remove(static_cast<TxPacket*>(data));
			return false;
		}

		bool hasJob() {
			return false;
		}

	public:
		TxPacket* getEgressPacket() {
/*			if(typename Os::IoJob* job = this->items.front()) {
				TxPacket* ret = reinterpret_cast<TxPacket*>(job->param);
				this->jobDone(job); 										// XXX
				TxPacket* ret = nullptr;
				return ret;
			}*/

			return nullptr;
		}
	} sender;

	virtual typename Os::IoChannel *getAllocator() override final {
		return &If::allocator;
	}

	virtual typename Os::IoChannel *getSender() override final {
		return &sender;
	}

	virtual typename Packet::Operations const *getStandardPacketOperations() override final {
		return &If::standardPacketOperations;
	}

	void init() {
		sender.init();
		If::init();
	}
};

template<class S, class... Args>
template<class C>
constexpr inline typename Network<S, Args...>::Interface *Network<S, Args...>::getIf() {
	return static_cast<TxBinder<C>*>(&state.interfaces);
}

template<class S, class... Args>
template<class C>
constexpr inline typename Network<S, Args...>::TxPacket *Network<S, Args...>::getEgressPacket() {
	return static_cast<TxBinder<C>*>(&state.interfaces)->sender.getEgressPacket();
}


#endif /* INTERFACES_H_ */
