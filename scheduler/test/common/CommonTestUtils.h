/*
 * CommonTestUtils.h
 *
 *  Created on: 2017.05.30.
 *      Author: tooma
 */

#ifndef COMMONTESTUTILS_H_
#define COMMONTESTUTILS_H_

#include "1test/Test.h"

#include <stdint.h>

#include "Platform.h"
#include "Scheduler.h"

#include "policy/RoundRobinPrioPolicy.h"
#include "policy/RealtimePolicy.h"

#include "TestStackPool.h"
#include "TestOsDefinitions.h"

#include "TestTask.h"

#include "SharedData.h"

class CommonTestUtils {
	static uint64_t iterationsPerMs;
	static void busyWorker(uint64_t iterations);
public:
	static uintptr_t startParam;
    static void (*registerIrq)(void (*)());

	static inline void busyWorkMs(uint32_t ms) {
	    busyWorker(iterationsPerMs * ms);
	}

	static inline void calibrate() {
        uint64_t trial = 10000;

        while(1) {
            auto startTicks = OsRr::getTick();
            busyWorker(trial);
            auto endTicks = OsRr::getTick();
            auto time = endTicks - startTicks;

            if(time > 16) {
                iterationsPerMs = trial / time;
                break;
            } else
                trial *= 2;
        }
	}

	template<class Os=OsRr>
	static void start() {
		Os::start(startParam);
	}
};


#endif /* COMMONTESTUTILS_H_ */
