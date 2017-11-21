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

struct DummyIf {
	inline static void enableTxIrq();
	inline static void disableTxIrq() {}
	inline static void enableRxIrq() {}
	inline static void disableRxIrq() {}
};

using Net = Network<OsRr,
		NetworkOptions::Interfaces<NetworkOptions::Set<DummyIf>>
>;

inline void DummyIf::enableTxIrq() {
	while(Net::getEgressPacket<DummyIf>());
}

#endif /* TESTNETDEFINITIONS_H_ */
