/*
 * PacketAssembler.h
 *
 *  Created on: 2018.01.20.
 *      Author: tooma
 */

#ifndef PACKETASSEMBLER_H_
#define PACKETASSEMBLER_H_

#include "Network.h"

template<class S, class... Args>
class Network<S, Args...>::PacketAssembler: public Packet
{
protected:
	Block* current;

public:
	inline void addBlock(Block* next) {
		current->setNext(next);
		current = next;
	}

	inline void addBlockByFinalInlineData(char* data, uint16_t length) {
		auto next = Block::fromInlineData(data);
		next->setSize(length);
		current->setNext(next);
		current = next;
	}

	inline void init(Block* first) {
		Packet::init(first);
		current = first;
	}

	inline void initByFinalInlineData(char* data, uint16_t length) {
		auto first = Block::fromInlineData(data);
		first->setSize(length);
		init(first);
	}

	inline void done() {
		current->setEndOfPacket(true);
		current->terminate();
	}
};

#endif /* PACKETASSEMBLER_H_ */
