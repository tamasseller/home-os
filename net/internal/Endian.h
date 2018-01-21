/*
 * Endian.h
 *
 *  Created on: 2017.11.28.
 *      Author: tooma
 */

#ifndef ENDIAN_H_
#define ENDIAN_H_

template<class S, class... Args>
inline constexpr uint16_t Network<S, Args...>::correctEndian(uint16_t arg) {
	return swapBytes ? static_cast<uint16_t>((arg >> 8) | (arg << 8)) : arg;
}

template<class S, class... Args>
inline constexpr uint32_t Network<S, Args...>::correctEndian(uint32_t arg) {
	return swapBytes ? correctEndian(static_cast<uint16_t>(arg >> 16))
			| correctEndian(static_cast<uint16_t>(arg)) << 16 : arg;
}

#endif /* ENDIAN_H_ */
