/*
 * AddressEthernet.h
 *
 *  Created on: 2017.11.26.
 *      Author: tooma
 */

#ifndef ADDRESSETHERNET_H_
#define ADDRESSETHERNET_H_

class AddressEthernet {
	template<int>
	struct ConstantContainer {
		static const AddressEthernet broadcast;
		static const AddressEthernet zero;
	};

public:
	static constexpr const AddressEthernet &broadcast = ConstantContainer<0>::broadcast;
	static constexpr const AddressEthernet &allZero = ConstantContainer<0>::zero;

	uint8_t bytes[6];

	static constexpr AddressEthernet make(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f){
		return AddressEthernet{{a, b, c, d, e, f}};
	}

	bool operator ==(const AddressEthernet& other) const {
		return memcmp(bytes, other.bytes, sizeof(bytes)) == 0;
	}

    bool operator !=(const AddressEthernet& other) const {
        return memcmp(bytes, other.bytes, sizeof(bytes)) != 0;
    }
};

static constexpr AddressEthernet broadcast = AddressEthernet::make(0xff, 0xff, 0xff, 0xff, 0xff, 0xff);
static constexpr AddressEthernet zero = AddressEthernet::make(0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

template<int dummy>
constexpr AddressEthernet AddressEthernet::ConstantContainer<dummy>::broadcast =
		AddressEthernet::make(0xff, 0xff, 0xff, 0xff, 0xff, 0xff);

template<int dummy>
constexpr AddressEthernet AddressEthernet::ConstantContainer<dummy>::zero =
		AddressEthernet::make(0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

#endif /* ADDRESSETHERNET_H_ */
