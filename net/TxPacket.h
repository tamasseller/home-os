/*
 * TxPacket.h
 *
 *  Created on: 2017.11.21.
 *      Author: tooma
 */

#ifndef TXPACKET_H_
#define TXPACKET_H_

template<class S, class... Args>
class Network<S, Args...>::TxPacket: public ChunkedPacket<TxPacket> {
    friend class ChunkedPacket<TxPacket>;
	size_t getSize() {
		return transmitBlockSize;
	}

	char* nextBlock() {
		auto currentBlock = reinterpret_cast<typename TxPool::Block*>(this->block());
		auto successorBlock = currentBlock->next;
		return reinterpret_cast<char*>(successorBlock);
	}

public:

	inline TxPacket(): ChunkedPacket<TxPacket>((char*)this, sizeof(*this)) {}

	inline virtual ~TxPacket() override final{
		auto first = reinterpret_cast<typename TxPool::Block*>(this);
		state.txPool.reclaim(first);
	}
};

#endif /* TXPACKET_H_ */
