/*
 * semihosting.h
 *
 *  Created on: 2017.05.29.
 *      Author: tooma
 */

#ifndef SEMIHOSTING_H_
#define SEMIHOSTING_H_

#include <stdint.h>

class Semihosting {
	static constexpr auto bufferSize = 64u;
	static char buffer[bufferSize];
	static uint32_t idx;
	static void flush();
protected:
	static void write(const char* val);
	static void write(unsigned int val);
};

#endif /* SEMIHOSTING_H_ */
