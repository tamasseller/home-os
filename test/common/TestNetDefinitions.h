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

#include "1test/Mock.h"

template<class Net, uint16_t blockMaxPayload>
struct DummyIf {
    static constexpr size_t arpCacheEntries = 8;
	static constexpr auto ethernetAddress = AddressEthernet::make(0xee, 0xee, 0xee, 0xee, 0xee, 0);
	static constexpr auto nRxBlocks = 4;

	static bool txEnabled, txDelayed;
	static size_t dummyPrefixSize;
	static char* rxTable[nRxBlocks];
	static int rxWriteIdx, rxReadIdx;

	inline static void disableTxIrq() {
		txEnabled = false;
	}

	inline static void init() {
		dummyPrefixSize = 0;
		txEnabled = txDelayed = false;
		rxWriteIdx = rxReadIdx = 0;
		Net::template getEthernetInterface<DummyIf>()->requestRxBuffers(nRxBlocks);
	}

	inline bool takeRxBuffer(char* data) {
		rxTable[rxWriteIdx] = data;
		rxWriteIdx = (rxWriteIdx + 1) % nRxBlocks;
		return true;
	}

	static inline char* receiveSome(const uint8_t *&p, uint16_t &length, uint16_t &run)
	{
		char* ret = rxTable[rxReadIdx];
		rxReadIdx = (rxReadIdx + 1) % nRxBlocks;

		run = (length < blockMaxPayload) ? length : blockMaxPayload;

		memcpy(ret, p, run);
		p += run;

		length = static_cast<uint16_t>(length - run);

		return ret;
	}

	template<class... C>
	static inline bool receive(C... c) {
		const uint8_t input[] = {static_cast<uint8_t>(c)...}, *p = input;
		uint16_t length = sizeof...(C), run;

		if((rxWriteIdx - rxReadIdx) == 1)
			return false;

		typename Net::PacketAssembler assembler;
		char* chunk = receiveSome(p, length, run);
		assembler.initByFinalInlineData(chunk, run);
		uint16_t nBlocksConsumed = 1;

		while(length) {
			char* chunk = receiveSome(p, length, run);
			assembler.addBlockByFinalInlineData(chunk, run);
			nBlocksConsumed++;
		}

		assembler.done();

		auto self = Net::template getEthernetInterface<DummyIf>();
		self->requestRxBuffers(nBlocksConsumed);

		if(!input[13]) /* 13: second byte of ethertype */
			self->ipPacketReceived(assembler);
		else
			self->arpPacketReceived(assembler);

		return true;
	}

	template<class... C>
	static inline void expectN(size_t n, C... c) {
		const uint8_t input[] = {static_cast<uint8_t>(c)...};
		const uint8_t *p = input;
		size_t l = sizeof...(c);

		uint32_t sum = 0;
		while(l--)
			sum = 31 * sum + *p++;

		while(n--)
			MOCK(DummyIf)::EXPECT(tx).withParam(sum);
	}

	static inline void processPackets() {
	    auto x = Net::template getEthernetInterface<DummyIf>();
	    while(auto* p = x->getCurrentTxPacket()) {
	        typename Net::PacketStream packet;
	        packet.init(*p);

	        uint32_t sum = 0;
	        size_t skip = dummyPrefixSize;
	        while(!packet.atEop()) {
	        	uint8_t b;
	        	packet.read8(b);

	        	if(skip)
	        		skip--;
	        	else
	        		sum = 31 * sum + b;
	        }

	        MOCK(DummyIf)::CALL(tx).withParam(sum);
	        x->packetTransmitted();
	    }
	}

	static inline void enableTxIrq() {
		txEnabled = true;
		if(!txDelayed)
			processPackets();
	}

	static inline void setDelayed(bool x) {
		txDelayed = x;
		if(!txDelayed && txEnabled)
			processPackets();
	}

	static inline void setFillSize(size_t size) {
		dummyPrefixSize = size - 14; /* 14: ethernet header size */
	}

	template<class T>
	static inline void preHeaderFill(T& packet) {
		for(auto i = 0u; i<dummyPrefixSize; i++)
			packet.write8(0);
	}
};

template<class Net, uint16_t n>
constexpr AddressEthernet DummyIf<Net, n>::ethernetAddress;

template<class Net, uint16_t n>
bool DummyIf<Net, n>::txEnabled;

template<class Net, uint16_t n>
bool DummyIf<Net, n>::txDelayed;

template<class Net, uint16_t n>
size_t DummyIf<Net, n>::dummyPrefixSize = 0;

template<class Net, uint16_t n>
char* DummyIf<Net, n>::rxTable[];
template<class Net, uint16_t n>
int DummyIf<Net, n>::rxWriteIdx = 0;
template<class Net, uint16_t n>
int DummyIf<Net, n>::rxReadIdx = 0;


using Ifs = NetworkOptions::Interfaces<
    NetworkOptions::Set<
        NetworkOptions::EthernetInterface<DummyIf>
    >
>;

using Net64 = Network<OsRr, Ifs, NetworkOptions::BufferSize<64>,
		NetworkOptions::TicksPerSecond<20>, NetworkOptions::UseDiagnosticCounters<true>>;

using Net43 = Network<OsRr, Ifs, NetworkOptions::BufferSize<43>,
		NetworkOptions::TicksPerSecond<20>, NetworkOptions::UseDiagnosticCounters<true>>;

template<class Net>
struct NetBuffers {
	static typename Net::Buffers buffers;
};

template<class Net>
typename Net::Buffers NetBuffers<Net>::buffers;

template<class Net>
struct NetworkTestAccessor: Net {
	static constexpr auto &pool = Net::state.pool;
	using Net::Pool;
	using Net::Chunk;
	using Net::Block;
	using Net::Packet;
	using Net::PacketChain;
	using Net::PacketBuilder;
	using Net::PacketAssembler;

	static void overrideHeaderSize(typename Net::Interface* interface, uint16_t headerSize) {
		interface->headerSize = headerSize;
	}
};

#endif /* TESTNETDEFINITIONS_H_ */
