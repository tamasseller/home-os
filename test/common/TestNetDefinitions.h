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

template<uint8_t id = 0>
class DummyIf {
	static constexpr size_t arpCacheEntries = 8;
	static constexpr AddressEthernet mac = AddressEthernet::make(0xee, 0xee, 0xee, 0xee, 0xee, id);

public:
	static constexpr size_t arpReqTimeout = 100;
	static constexpr auto& ethernetAddress = mac;
	inline static void enableTxIrq();
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
			MOCK(DummyIf)::EXPECT(tx).withParam(id).withParam(sum);
	}

};

template<uint8_t id>
constexpr AddressEthernet DummyIf<id>::mac;

using Net = Network<OsRr,
		NetworkOptions::Interfaces<
		    NetworkOptions::Set<
		        NetworkOptions::EthernetInterface<DummyIf<0>>
            >
        >,
		NetworkOptions::ArpRequestRetry<3>
>;

template<class Net>
struct NetBuffers {
	static typename Net::Buffers buffers;
};

template<class Net>
typename Net::Buffers NetBuffers<Net>::buffers;

template<uint8_t id>
inline void DummyIf<id>::enableTxIrq() {
	auto x = Net::geEthernetInterface<DummyIf<id>>()->getTxInfoProvider();
	while(Net::PacketTransmissionRequest* p = x->getCurrentPacket()) {
		Net::PacketStream packet;
		packet.init(*p);

		uint32_t sum = 0;
		while(!packet.atEop())
		    sum = 31 * sum + packet.read8();

		MOCK(DummyIf)::CALL(tx).withParam(id).withParam(sum);
		x->packetTransmitted();
	}
}

struct NetworkTestAccessor: Net {
	static constexpr auto &pool = Net::state.pool;
	using Net::Pool;
	using Net::Chunk;
	using Net::Block;
	using Net::Packet;
	using Net::PacketAssembler;
	using Net::PacketDisassembler;
	using Net::PacketBuilder;
};

#endif /* TESTNETDEFINITIONS_H_ */
