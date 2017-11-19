/*
 * AddressIpv4.h
 *
 *  Created on: 2017.11.19.
 *      Author: tooma
 */

#ifndef ADDRESSIP4_H_
#define ADDRESSIP4_H_

#include <stdint.h>

struct AddressIp4 {
	uint32_t addr;

	static constexpr AddressIp4 make(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
		return AddressIp4{((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)c << 8) | ((uint32_t)d)};
	}
};

#endif /* ADDRESSIP4_H_ */
