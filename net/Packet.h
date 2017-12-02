/*
 * Packet.h
 *
 *  Created on: 2017.11.19.
 *      Author: tooma
 */

#ifndef PACKET_H_
#define PACKET_H_

#include "Network.h"

template<>
class Block {
	friend class BufferPool;

public:
	/**
	 * Optional destructor for indirect blocks.
	 */
	typedef void (*Destructor)(void*, char*, char*);

private:

	/*
	 * Helper types.
	 */
	struct Header {
		int16_t nextOffset;
		uint16_t sizeAndFlags = 0;
	};

	struct IndirectEntry {
		void *user;
		char *start;
		size_t size;
		Destructor destructor;
	};

	/*
	 * Constant definitions
	 */
	static constexpr size_t dataSize = sizeof(Block) - sizeof(Header);
	static constexpr uint16_t endOfPacket = 0x8000;

	/*
	 * Helper header structure
	 */
	static_assert(nBlocks < 32768, "Too many blocks requested");
	static_assert(blockSize < 16384, "Pool blocks are too big");
	static_assert(dataSize >= sizeof(Entry), "Pool blocks are too small");

	/*
	 * Data members.
	 */

	Header header;

	union {
		/// In-line data storage area.
		char bytes[blockSize - sizeof(Header)];

		/// Indirect data storage (with optional destructor).
		IndirectEntry entry;
	};

public:
	/**
	 * Read the next block.
	 *
	 * The next block must be inside the pool.
	 */
	inline Block *getNext() {
		if(!header.nextOffset)
			return nullptr;

		return this + header.nextOffset;
	}

	/**
	 * Set the next block.
	 *
	 * The next block must be inside the pool.
	 */
	inline void setNextNonNull(Block *block) {
		header.nextOffset = static_cast<uint16_t>(block - this);
	}

	/**
	 * Terminate chain of blocks.
	 *
	 * Sets the block in such a state that it indicates
	 * that there is no next block.
	 */
	inline void terminate() {
		header.nextOffset = 0;
	}

	/**
	 * Get the size of the stored data.
	 */
	inline uint16_t getSize() {
		if((header.sizeAndFlags & endOfPacket) != 0)
			return header.sizeAndFlags & ~endOfPacket;
		else
			return entry.size;
	}

	/**
	 * Set the size of the contained data (implies in-line storage).
	 */
	inline void setSize(uint16_t size) {
		header.sizeAndFlags = (header.sizeAndFlags & endOfPacket) | (size & endOfPacket);
	}

	/**
	 * Check whether the block contains the data in-line or an indirect reference to it.
	 */
	inline bool isIndirect() {
		return (header.sizeAndFlags & ~endOfPacket) == 0;
	}

	/**
	 * Set the indirect content.
	 */
	inline void setIndirect(char *start, size_t size, Destructor destructor = nullptr, void *user = nullptr) {
		entry.user = user;
		entry.start = start;
		entry.size = size;
		entry.destructor = destructor;
	}

	/**
	 * Check if this block is flagged as the end of a packet.
	 */
	inline bool isEndOfPacket() {
		return header.sizeAndFlags & endOfPacket;
	}

	/**
	 * Set or reset the end of packet flag.
	 */
	inline void setEndOfPacket(bool x) {
		header.sizeAndFlags = x ? (header.sizeAndFlags | endOfPacket) : (header.sizeAndFlags & ~endOfPacket);
	}

	/**
	 * Get the pointer to the data (in-line or indirect).
	 */
	inline char* data() {
		if((header.sizeAndFlags & endOfPacket) != 0)
			return &bytes[0];
		else
			return entry.start;
	}

	/**
	 * Call the destructor if it is an indirect block and there is one.
	 */
	inline void dispose() {
		if(isIndirect()) {
			if(entry.destructor) {
				entry.destructor(entry.user, entry.start, entry.size);
			}
		}
	}
};


template<class S, class... Args>
class Network<S, Args...>::Packet
{
	friend Network<S, Args...>;

	typename State::Pool::Block* first;
};

template<class S, class... Args>
class Network<S, Args...>::PacketWriter
{
	friend Network<S, Args...>;
	Packet& packet;

public:
	inline bool copyIn(const char* data, size_t dataLength) {
	/*	while(dataLength) {
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
		}*/

		return true;
	}

	inline bool copyIn32(uint32_t data) {
		return copyIn((const char*)&data, 4); // TODO optimize
	}
};

template<class S, class... Args>
class Network<S, Args...>::PacketReader
{

};


#endif /* PACKET_H_ */
