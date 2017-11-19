/*
 * Chunk.h
 *
 *  Created on: 2017.11.19.
 *      Author: tooma
 */

#ifndef CHUNK_H_
#define CHUNK_H_

#include <stddef.h>

class Chunk {
	char *startPtr, *endPtr;
public:

	inline Chunk(char *start, char *end): startPtr(start), endPtr(end) {}

	inline size_t length() {
		return endPtr - startPtr;
	}

	inline char* begin() {
		return startPtr;
	}

	inline char* end() {
		return endPtr;
	}
};



#endif /* CHUNK_H_ */
