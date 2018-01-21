/*
 * IcmpReceiver.h
 *
 *  Created on: 2018.01.21.
 *      Author: tooma
 */

#ifndef ICMPRECEIVER_H_
#define ICMPRECEIVER_H_

#include "Network.h"

template<class S, class... Args>
class Network<S, Args...>::IcmpReceiver:
    public Os::template IoRequest<IpRxJob<
    	IcmpReceiver, typename IcmpCore::InputChannel>,
    	&IpRxJob<IcmpReceiver, typename IcmpCore::InputChannel>::onBlocking>
{
	friend typename IcmpReceiver::IoRequest::IpRxJob;

    inline void preprocess(Packet) {
    	state.increment(&DiagnosticCounters::Icmp::inputProcessed);
    }

public:
	void init() {
	    IcmpReceiver::IpRxJob::init();
		IcmpReceiver::IoRequest::init();
	}

	bool receive() {
		return this->launch(&IcmpReceiver::IpRxJob::startReception, &state.icmpCore.inputChannel);
	}

	void close() {
		this->cancel();
		this->wait();
	}
};

#endif /* ICMPRECEIVER_H_ */
