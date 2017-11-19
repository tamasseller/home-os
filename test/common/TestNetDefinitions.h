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

};

using Net = Network<OsRr,
		NetworkOptions::Interfaces<NetworkOptions::Set<DummyIf>>
>;


#endif /* TESTNETDEFINITIONS_H_ */
