/*
 * Packet.h
 *
 *  Created on: 2017.11.19.
 *      Author: tooma
 */

#ifndef PACKET_H_
#define PACKET_H_

#include "Chunk.h"

// TODO change visibility

class Packet
{
public:
	struct Point {
		void* block;
		size_t offset;
	};

	struct Operations {
		Chunk (*nextChunk)(Packet*) = 0;
		void (*dispose)(Packet*) = 0;
	};

	Operations *ops;
	Point cursor;

	void init(Operations *ops, char* block, size_t offset) {
		cursor.block = block;
		cursor.offset = offset;
	}

	inline Point getPoint() const {
		return cursor;
	}

	inline void resetTo(const Point& point) {
		cursor = point;
	}

	inline void leaveAt(size_t newOffset) {
		cursor.offset += newOffset;
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

	Chunk nextChunk() {
		return ops->nextChunk(this);
	}

	inline void dispose() {
		ops->dispose(this);
	}
};

template<class Child>
class ChunkedPacket
{
	static Chunk nextChunk(Packet* self)
	{
		if(self->cursor.block) {
			const size_t size = Child::getSize(self->cursor.block);

			if(self->cursor.offset < size) {
				char* data = Child::getData(self->cursor.block);
				return Chunk(data + self->cursor.offset, data + (size - self->cursor.offset));
			} else {
				self->cursor.block = Child::nextBlock(self->cursor.block);
				self->cursor.offset = 0;

				if(self->cursor.block) {
					char* data = Child::getData(self->cursor.block);
					return Chunk(data, data + size);
				}
			}
		}

		return Chunk(nullptr, nullptr);
	}

	static void dispose(Packet* packet) {
		Child::dispose(packet);
	}

public:
	static constexpr Packet::Operations operations = Packet::Operations{&ChunkedPacket::nextChunk, &ChunkedPacket::dispose};
};

template<class Child>
constexpr Packet::Operations ChunkedPacket<Child>::operations;


#endif /* PACKET_H_ */
