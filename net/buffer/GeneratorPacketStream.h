/*
 * GeneratorPacketStream.h
 *
 *  Created on: 2018.01.20.
 *      Author: tooma
 */

#ifndef GENERATORPACKETSTREAM_H_
#define GENERATORPACKETSTREAM_H_

#include "Network.h"

template<class S, class... Args>
class Network<S, Args...>::GeneratorPacketStream: public PacketStreamBase<ChecksumObserver>
{
public:
	inline GeneratorPacketStream() = default;
	inline GeneratorPacketStream(const Packet& p) {
		this->init(p);
	}

	inline GeneratorPacketStream(const Packet& p, uint16_t offset) {
		this->init(p, offset);
	}

};

#endif /* GENERATORPACKETSTREAM_H_ */
