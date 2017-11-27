/*******************************************************************************
 *
 * Copyright (c) 2017 Seller Tamás. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *******************************************************************************/

#ifndef TESTSUITE_H_
#define TESTSUITE_H_

#include "1test/TestPlugin.h"
#include "1test/TestRunner.h"

#include "CommonTestUtils.h"
#include "OsTestPlugin.h"

template<void (*testProgressReport)(int), void (testStartedReport)(), void (registerIrqPtr)(void (*)())>
class TestSuite {
	static class Output: public pet::TraceOutput {
		virtual void reportProgress() {
			testProgressReport(nDots);
			pet::TraceOutput::reportProgress();
		}
	} output;

	static struct Calibrator: OsRr::Task {
	    inline void run() {
	        CommonTestUtils::calibrate();
	    }

	    inline void start() {
	    	this->OsRr::Task::template start<Calibrator, &Calibrator::run>(StackPool::get(), testStackSize);
	    }
	} calibrator;

public:

	static int runTests(uintptr_t tickFreq) {
		CommonTestUtils::startParam = tickFreq;

		calibrator.start();
		CommonTestUtils::start();
		StackPool::clear();

		OsTestPlugin plugin;
		pet::TestRunner::installPlugin(&plugin);

		testStartedReport();

		CommonTestUtils::registerIrq = registerIrqPtr;

		return pet::TestRunner::runAllTests(&output);
	}
};

template<void (*testProgressReport)(int), void (testStartedReport)(), void (registerIrq)(void (*)())>
typename TestSuite<testProgressReport, testStartedReport, registerIrq>::Output
TestSuite<testProgressReport, testStartedReport, registerIrq>::output;

template<void (*testProgressReport)(int), void (testStartedReport)(), void (registerIrq)(void (*)())>
typename TestSuite<testProgressReport, testStartedReport, registerIrq>::Calibrator
    TestSuite<testProgressReport, testStartedReport, registerIrq>::calibrator;

#endif /* TESTSUITE_H_ */
