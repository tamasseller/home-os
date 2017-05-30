/*
 * semihosting.cpp
 *
 *  Created on: 2017.05.29.
 *      Author: tooma
 */

#ifndef SEMIHOSTING_CPP_
#define SEMIHOSTING_CPP_

#include "semihosting.h"

#include "algorithm/Str.h"

char Semihosting::buffer[Semihosting ::bufferSize];
uint32_t Semihosting::idx = 0;

void Semihosting::write(const char* c)
{
	while(*c) {
		buffer[idx++] = *c;

		if(idx == sizeof(buffer) || *c == '\n')
			flush();

		c++;
	}
}

static char temp[16];

void Semihosting::write(unsigned int val) {
	pet::Str::utoa<10>(val, temp, sizeof(temp));
	write(temp);
}

void Semihosting::flush() {
	uint32_t msg[3];
	msg[0] = 2;
	msg[1] = (uint32_t)buffer;
	msg[2] = idx;
	asm("mov r0, #5;"
		"mov r1, %[msg];"
		"bkpt #0xab"
		: // no output
		: [msg] "r" (msg)
		: "r0", "r1", "memory");

	idx = 0;
}

#endif /* SEMIHOSTING_CPP_ */
