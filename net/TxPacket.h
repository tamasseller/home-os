/*
 * TxPacket.h
 *
 *  Created on: 2017.11.26.
 *      Author: tooma
 */

#ifndef TXPACKET_H_
#define TXPACKET_H_

#include "Network.h"

template<class S, class... Args>
struct Network<S, Args...>::TxPacket: Packet, Os::IoJob::Data {
	TxPacket *next;
};

#endif /* TXPACKET_H_ */
