/*
 * CommonTestUtils.h
 *
 *  Created on: 2017.05.30.
 *      Author: tooma
 */

#ifndef COMMONTESTUTILS_H_
#define COMMONTESTUTILS_H_

#include <stdint.h>

class CommonTestUtils {
	static uint64_t iterationsPerMs;
public:
	static uintptr_t startParam;

	static void busyWork(uint64_t iterations);
	static void calibrate();
	static uint64_t getIterations(uintptr_t ms);
	static void start();
};

template<uintptr_t size>
class SharedData {
	uint16_t data[size];
	bool error = false;
public:
	inline void update() {
		for(int i = sizeof(data)/sizeof(data[0]) - 1; i >= 0; i--) {
			if(data[i] != data[0])
				error = true;

			data[i]++;
		}
	}

	inline bool check(uint16_t value) {
		for(int i = sizeof(data)/sizeof(data[0]) - 1; i >= 0; i--)
			if(data[i] != value)
				return false;

		return !error;
	}
};

#endif /* COMMONTESTUTILS_H_ */
