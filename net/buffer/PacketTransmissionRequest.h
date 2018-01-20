/*
 * PacketTransmissionRequest.h
 *
 *  Created on: 2018.01.20.
 *      Author: tooma
 */

#ifndef PACKETTRANSMISSIONREQUEST_H_
#define PACKETTRANSMISSIONREQUEST_H_

#include "Network.h"

template<class S, class... Args>
struct Network<S, Args...>::PacketTransmissionRequest: Packet, Os::IoJob::Data {
	PacketTransmissionRequest *next;

	void init(const Packet& packet) {
		*static_cast<Packet*>(this) = packet;
	}
};

#endif /* PACKETTRANSMISSIONREQUEST_H_ */
