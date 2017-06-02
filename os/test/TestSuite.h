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

class OsTestPlugin: public pet::TestPlugin
{
	virtual void beforeTest();
	virtual void afterTest();
public:
	inline virtual ~OsTestPlugin() {}
};

extern struct Calibrator: TestTask<Calibrator>{
	void run();
} calibrator;

template<void (*platformTestProgressReport)(int), void (platformTestStartedReport)()>
class TestSuite {
	static class Output: public pet::TraceOutput {
		virtual void reportProgress() {
			platformTestProgressReport(nDots);
			pet::TraceOutput::reportProgress();
		}
	} output;

public:
	static bool runTests(uintptr_t tickFreq) {
		CommonTestUtils::startParam = tickFreq;

		calibrator.start();
		CommonTestUtils::start();

		OsTestPlugin plugin;
		pet::TestRunner::installPlugin(&plugin);

		return pet::TestRunner::runAllTests(&output) == 0;
	}
};

template<void (*platformTestProgressReport)(int), void (platformTestStartedReport)()>
typename TestSuite<platformTestProgressReport, platformTestStartedReport>::Output
TestSuite<platformTestProgressReport, platformTestStartedReport>::output;

#endif /* TESTSUITE_H_ */
