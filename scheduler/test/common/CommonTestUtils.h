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

#include "algorithm/Str.h"

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
	static void start(const char* expectedError = nullptr) {
		const char* result = Os::start(startParam);
		if(expectedError)
			CHECK(pet::Str::cmp(result, expectedError) == 0);
		else
			CHECK(result == nullptr);
	}
};

template<class Os> struct OsInternalTester: Os {
	using typename Os::Timeout;
	using typename Os::Sleeper;
};

#endif /* COMMONTESTUTILS_H_ */
