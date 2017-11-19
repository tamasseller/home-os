/*
 * Packet.h
 *
 *  Created on: 2017.11.19.
 *      Author: tooma
 */

#ifndef PACKET_H_
#define PACKET_H_

#include "Chunk.h"

class Packet
{
public:
	class Point {
		friend Packet;
		char* block;
		size_t offset;
	};

protected:

	Point cursor;
	inline char* &block() {return cursor.block;}
	inline size_t &offset() {return cursor.offset;}

public:

	inline Point getPoint() const {
		return cursor;
	}

	inline void resetTo(const Point& point) {
		cursor = point;
	}

	inline void leaveAt(size_t newOffset) {
		offset() += newOffset;
	}

	bool copyIn(const char* data, size_t dataLength) {
		while(dataLength) {
			Chunk chunk = nextChunk();
			const size_t length = chunk.length();

			if(!length)
				return false;

			if(length < dataLength) {
				memcpy((void*)chunk.begin(), data, length);
				dataLength -= length;
				data += length;
				leaveAt(dataLength);
			} else {
				memcpy((void*)chunk.begin(), data, dataLength);
				leaveAt(dataLength);
				break;
			}
		}

		return true;
	}

	virtual Chunk nextChunk() = 0;

	inline virtual ~Packet() {}
};

class ChunkedPacket: public Packet
{
	virtual size_t getSize() = 0;
	virtual char* nextBlock() = 0;

	virtual inline Chunk nextChunk() override
	{
		if(block()) {
			const size_t size = getSize();

			if(offset() < size) {
				return Chunk(block() + offset(), block() + (size - offset()));
			} else {
				auto newBlock = nextBlock();
				block() = newBlock;
				offset() = 0;

				if(newBlock)
					return Chunk(newBlock, newBlock + size);
			}
		}

		return Chunk(nullptr, nullptr);
	}

protected:

	ChunkedPacket(char* newBlock, size_t newOffset) {
		block() = newBlock;
		offset() = newOffset;
	}

public:

	inline virtual ~ChunkedPacket() {}

};


#endif /* PACKET_H_ */
