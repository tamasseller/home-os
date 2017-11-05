/*
 * TestSuite.h
 *
 *  Created on: 2017.05.30.
 *      Author: tooma
 */

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

	static struct Calibrator: TestTask<Calibrator>{
	    inline void run() {
	        CommonTestUtils::calibrate();
	    }
	} calibrator;

public:
	static int runTests(uintptr_t tickFreq) {
		CommonTestUtils::startParam = tickFreq;

		calibrator.start();
		CommonTestUtils::start();

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
