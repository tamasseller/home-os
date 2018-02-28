/*
 * Access.h
 *
 *  Created on: 2018.02.28.
 *      Author: tooma
 */

#ifndef ACCESS_H_
#define ACCESS_H_

namespace home {

static inline uint16_t readUnaligned16(const char* ptr) {
	return *reinterpret_cast<const uint16_t*>(ptr);
}

static inline uint32_t readUnaligned32(const char* ptr) {
	return *reinterpret_cast<const uint32_t*>(ptr);
}

static inline void writeUnaligned16(char* ptr, uint16_t data) {
	*reinterpret_cast<uint16_t*>(ptr) = data;
}

static inline void writeUnaligned32(char* ptr, uint32_t data) {
	*reinterpret_cast<uint32_t*>(ptr) = data;
}

static inline uint16_t reverseBytes16(uint16_t data) {
	return __builtin_bswap16(data);
}

static inline uint32_t reverseBytes32(uint32_t data) {
	return __builtin_bswap32(data);
}

}

#endif /* ACCESS_H_ */
