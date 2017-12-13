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
	template<int>
	struct ConstantContainer {
		static const AddressIp4 broadcast;
		static const AddressIp4 zero;
	};

public:
	static constexpr const AddressIp4 &broadcast = ConstantContainer<0>::broadcast;
	static constexpr const AddressIp4 &allZero = ConstantContainer<0>::zero;

	uint32_t addr;

	static constexpr AddressIp4 make(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
		return AddressIp4{((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)c << 8) | ((uint32_t)d)};
	}

    constexpr AddressIp4 operator/(uint8_t mask) const {
        return mask ? AddressIp4{addr & ~((1 << (32 - mask)) - 1)} : AddressIp4::allZero;
    }

	bool operator ==(const AddressIp4& other) const {
	    return addr == other.addr;
	}

    bool operator !=(const AddressIp4& other) const {
        return addr != other.addr;
    }
};

template<int dummy>
constexpr AddressIp4 AddressIp4::ConstantContainer<dummy>::broadcast =
		AddressIp4::make(255, 255, 255, 255);

template<int dummy>
constexpr AddressIp4 AddressIp4::ConstantContainer<dummy>::zero =
		AddressIp4::make(0, 0, 0, 0);

#endif /* ADDRESSIP4_H_ */
