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

template<class Child>
class ChunkedPacket: public Packet
{
	virtual inline Chunk nextChunk() override
	{
	    auto self = static_cast<Child*>(this);

		if(this->block()) {
			const size_t size = self->getSize();

			if(offset() < size) {
				return Chunk(this->block() + this->offset(), this->block() + (size - this->offset()));
			} else {
				auto newBlock = self->nextBlock();
				this->block() = newBlock;
				this->offset() = 0;

				if(newBlock)
					return Chunk(newBlock, newBlock + size);
			}
		}

		return Chunk(nullptr, nullptr);
	}

protected:

	ChunkedPacket(char* newBlock, size_t newOffset) {
		this->block() = newBlock;
		this->offset() = newOffset;
	}
};


#endif /* PACKET_H_ */
