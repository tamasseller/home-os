/*
 * TestMain.cpp
 *
 *  Created on: 2017.05.29.
 *      Author: tooma
 */

#include "TestSuite.h"

Calibrator calibrator;

void Calibrator::run() {
	CommonTestUtils::calibrate();
}

void OsTestPlugin::beforeTest() {
	StackPool::clear();
}

void OsTestPlugin::afterTest() {
	CommonTestUtils::registerIrq(nullptr);
}
