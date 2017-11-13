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
