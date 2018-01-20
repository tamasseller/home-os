/*
 * ValidatorPacketStream.h
 *
 *  Created on: 2018.01.20.
 *      Author: tooma
 */

#ifndef VALIDATORPACKETSTREAM_H_
#define VALIDATORPACKETSTREAM_H_

#include "Network.h"

template<class S, class... Args>
class Network<S, Args...>::ValidatorPacketStream: public PacketStreamBase<ChecksumObserver>
{
public:
	inline ValidatorPacketStream() = default;
	inline ValidatorPacketStream(const Packet& p) {
		this->init(p);
	}

	inline ValidatorPacketStream(const Packet& p, uint16_t offset) {
		this->init(p, offset);
	}

};

#endif /* VALIDATORPACKETSTREAM_H_ */
