/*
 * AddressEthernet.h
 *
 *  Created on: 2017.11.26.
 *      Author: tooma
 */

#ifndef ADDRESSETHERNET_H_
#define ADDRESSETHERNET_H_

struct AddressEthernet {
	uint8_t bytes[6];

	static constexpr AddressEthernet make(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f){
		return AddressEthernet{{a, b, c, d, e, f}};
	}

	bool operator ==(const AddressEthernet& other) const {
	    return bytes == other.bytes;
	}

    bool operator !=(const AddressEthernet& other) const {
        return bytes  != other.bytes ;
    }
};

#endif /* ADDRESSETHERNET_H_ */
