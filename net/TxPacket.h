/*
 * TxPacket.h
 *
 *  Created on: 2017.11.21.
 *      Author: tooma
 */

#ifndef TXPACKET_H_
#define TXPACKET_H_

#include "Network.h"

template<class S, class... Args>
class Network<S, Args...>::TxPacket: public Os::IoJob::Data, public Packet {
    friend class Network<S, Args...>;
    friend class pet::LinkedList<TxPacket>;
    TxPacket *next;
};

#endif /* TXPACKET_H_ */
