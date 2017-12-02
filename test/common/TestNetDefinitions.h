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
};

template<uint8_t id>
constexpr AddressEthernet DummyIf<id>::mac;

using Net = Network<OsRr,
		NetworkOptions::Interfaces<NetworkOptions::Set<DummyIf<0>>>,
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
	auto x = Net::getIf<DummyIf<id>>()->getTxInfoProvider();
	while(auto p = x->getCurrentPacket()) {
		/*for(p.get)
		MOCK(DummyIf)::CALL(tx).withParam(id).withParam();*/
		x->packetTransmitted();
	}
}

#endif /* TESTNETDEFINITIONS_H_ */
