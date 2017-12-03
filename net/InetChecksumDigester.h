/*
 * InetChecksumDigester.h
 *
 *  Created on: 2017.12.03.
 *      Author: tooma
 */

#ifndef INETCHECKSUMDIGESTER_H_
#define INETCHECKSUMDIGESTER_H_

#include <iostream>
#include <string.h>
#include <arpa/inet.h>

class InetChecksumDigester {
	uint32_t state = 0;

	constexpr static inline uint32_t fold(uint32_t x) {
		return (x & 0xffff) + (x >> 16);
	}

	constexpr static inline uint16_t reduce(uint32_t x) {
		return static_cast<uint16_t>(fold(fold(x)));
	}

	constexpr static inline uint16_t swap(uint16_t x) {
		return static_cast<uint16_t>((x << 8) | (x >> 8));
	}

public:
	/*
	 * |ab|cd|ef| ->  ab+cd+ef -> a, c, e + b, d, f
	 * |ab|cd|e0| ->  ab+cd+e0 -> a, c, e + b, d, 0
	 * a|bc|de|f  ->  swap(bc+de+fa) -> swap(b, d, f + c, e, a)
	 * a|bc|de|0  ->  swap(bc+de+0a) -> swap(b, d, 0 + c, e, a)
	 */
	void consume(const char* data, size_t length, bool oddOffset = false)
	{
		if(!length)
			return;

		uint32_t sum = 0;
		uint16_t temp = 0;
		bool flipped = (reinterpret_cast<uintptr_t>(data) & 1);

		if(flipped) {
			length--;
			reinterpret_cast<uint8_t*>(&temp)[1] = *data++;
		}

		const uint16_t* aligned = reinterpret_cast<const uint16_t*>(data);
		const uint16_t* const limit = aligned + (length >> 1);

		while(aligned < limit) {
			sum += *aligned++;
		}

		if(length & 1)
			reinterpret_cast<uint8_t*>(&temp)[0] = *reinterpret_cast<const uint8_t*>(aligned);

		sum += temp;

		uint16_t result = reduce(sum);

		if(flipped ^ oddOffset)
			result = swap(result);

		state += result;
	}

	void patch(uint16_t old, uint16_t updated) {
		state += updated - old;
	}

	uint16_t result() {
		return static_cast<uint16_t>(~reduce(state));
	}
};

#endif /* INETCHECKSUMDIGESTER_H_ */
