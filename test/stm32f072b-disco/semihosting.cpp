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
