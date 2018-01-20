/*
 * PacketStream.h
 *
 *  Created on: 2017.12.03.
 *      Author: tooma
 */

#ifndef PACKETSTREAM_H_
#define PACKETSTREAM_H_

#include "Network.h"

template<class S, class... Args>
template<class>
struct Network<S, Args...>::NullObserver {
	inline void observeFirstBlock() {}
	inline void observeInternalBlock() {}
};

template<class S, class... Args>
class Network<S, Args...>::PacketStream: public PacketStreamBase<NullObserver> {
public:
	inline PacketStream() = default;
	inline PacketStream(const Packet& p) {
		this->init(p);
	}
};

#endif /* PACKETSTREAM_H_ */
