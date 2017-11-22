/*
 * Interfaces.h
 *
 *  Created on: 2017.11.21.
 *      Author: tooma
 */

#ifndef INTERFACES_H_
#define INTERFACES_H_

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
template<class If>
class Network<S, Args...>::TxBinder: Network<S, Args...>::Os::template IoChannelBase<TxBinder<If>> {
	friend class Network<S, Args...>;
	friend class TxBinder::IoChannelBase;

	void enableProcess() {If::enableTxIrq();}
	void disableProcess() {If::disableTxIrq();}
	bool addJob(typename Os::IoJob* job) {return this->jobs.addBack(job);}
	bool removeJob(typename Os::IoJob* job) {return this->jobs.remove(job);}

	static bool done(typename Os::IoJob* item, typename Os::IoJob::Result result, void (*hook)(typename Os::IoJob*)) {
	    reinterpret_cast<TxPacket*>(item->param)->~TxPacket();
		return false;
	}

	TxPacket* getEgressPacket() {
		if(typename Os::IoJob* job = this->jobs.front()) {
			TxPacket* ret = reinterpret_cast<TxPacket*>(job->param);
			this->jobDone(job); 										// XXX
			return ret;
		}

		return nullptr;
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
	return static_cast<TxBinder<C>*>(&state.interfaces)->getEgressPacket();
}


#endif /* INTERFACES_H_ */
