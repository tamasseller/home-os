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

template<class Net>
struct DummyIf {
    static constexpr size_t arpCacheEntries = 8;
	static constexpr auto ethernetAddress = AddressEthernet::make(0xee, 0xee, 0xee, 0xee, 0xee, 0);
	inline static void disableTxIrq() {}
	inline static void init() {}

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

	inline void enableTxIrq() {
	    auto x = Net::template geEthernetInterface<DummyIf>();
	    while(auto* p = x->getCurrentTxPacket()) {
	        typename Net::PacketStream packet;
	        packet.init(*p);

	        uint32_t sum = 0;
	        while(!packet.atEop())
	            sum = 31 * sum + packet.read8();

	        MOCK(DummyIf)::CALL(tx).withParam(sum);
	        x->packetTransmitted();
	    }
	}
};

template<class Net>
constexpr AddressEthernet DummyIf<Net>::ethernetAddress;

using Ifs = NetworkOptions::Interfaces<
    NetworkOptions::Set<
        NetworkOptions::EthernetInterface<DummyIf>
    >
>;

using Net64 = Network<OsRr, Ifs, NetworkOptions::BufferSize<64>>;
using Net43 = Network<OsRr, Ifs, NetworkOptions::BufferSize<43>>;

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
	using Net::PacketAssembler;
	using Net::PacketDisassembler;
	using Net::TxPacketBuilder;
	using Net::RxPacketBuilder;
};

#endif /* TESTNETDEFINITIONS_H_ */
