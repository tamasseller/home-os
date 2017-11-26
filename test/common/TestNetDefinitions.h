/*
 * TestNetDefinitions.h
 *
 *  Created on: 2017.11.19.
 *      Author: tooma
 */

#ifndef TESTNETDEFINITIONS_H_
#define TESTNETDEFINITIONS_H_

#include "CommonTestUtils.h"
#include "Network.h"

template<int id = 0>
class DummyIf {
	static constexpr size_t txBufferCount = 64;
	static constexpr size_t txBufferSize = 64;
	static constexpr size_t arpCacheEntries = 8;

	struct TxHeader {
		char dummyByte;
		TxHeader* next;

		static inline TxHeader* &accessNext(TxHeader* header) {
			return header->next;
		}
	};

	static struct Pools {
		typedef BufferPool<OsRr, txBufferCount, txBufferSize, TxHeader, &TxHeader::accessNext> Tx;
		Tx txPool;
		typename Tx::Storage txStorage;
	} pools;

	struct TxChunkInfo {
		static inline size_t getSize(void *block) {
			return txBufferSize;
		}

		static inline char* getData(void *block) {
			return &reinterpret_cast<typename Pools::Tx::Block*>(block)->data[0];
		}

		static inline void *nextBlock(void *block) {
			return reinterpret_cast<typename Pools::Tx::Block*>(block)->getNext();
		}

		static inline void dispose(void *block) {
			return pools.txPool.reclaim(reinterpret_cast<typename Pools::Tx::Block*>(block));
		}
	};

public:
	static constexpr size_t arpReqTimeout = 100;
	static constexpr OsRr::IoChannel& allocator = pools.txPool;
	static constexpr auto& standardPacketOperations = ChunkedPacket<TxChunkInfo>::operations;
	inline static void enableTxIrq();
	inline static void disableTxIrq() {}
	inline static void init() { pools.txPool.init(pools.txStorage); }
};

template<int id>
typename DummyIf<id>::Pools DummyIf<id>::pools;


using Net = Network<OsRr,
		NetworkOptions::Interfaces<NetworkOptions::Set<DummyIf<0>>>,
		NetworkOptions::ArpRequestRetry<3>
>;

template<int id>
inline void DummyIf<id>::enableTxIrq() {
	auto x = Net::getIf<DummyIf<id>>()->getTxInfoProvider();
	while(auto p = x->getCurrentPacket()) {
		x->packetTransmitted();
	}
}

#endif /* TESTNETDEFINITIONS_H_ */
