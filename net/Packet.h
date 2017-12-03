/*
 * Packet.h
 *
 *  Created on: 2017.11.19.
 *      Author: tooma
 */

#ifndef PACKET_H_
#define PACKET_H_

#include "Network.h"

template<class S, class... Args>
struct Network<S, Args...>::Chunk {
	char *start, *end;

	inline size_t length() {
		return end-start;
	}
};


template<class S, class... Args>
class Network<S, Args...>::Block {
public:
	/**
	 * Optional destructor for indirect blocks.
	 */
	typedef void (*Destructor)(void*, char*, size_t);

private:

	/*
	 * Helper types.
	 *
	 * The initial state indicates that:
	 *
	 *  - The block is in-line type, with no data yet,
	 *  - There is no next block, but
	 *  - The current one is not the end of the packet.
	 *
	 * This intermediate state is changed by the PacketWriter either by
	 * adding more blocks or flagging the block as the last one of the packet.
	 */
	struct Header {
		int16_t nextOffset = 0;
		uint16_t sizeAndFlags = 0;
	};

	struct IndirectEntry {
		void *user;
		char *start;
		uint16_t size;
		Destructor destructor;
	};

	/*
	 * Constant definitions
	 */
public:
	static constexpr size_t dataSize = bufferSize - sizeof(Header);

private:
	static constexpr uint16_t endOfPacket = 0x8000;

	static_assert(dataSize >= sizeof(IndirectEntry), "Pool blocks are too small");

	/*
	 * Data members.
	 */

	Header header;

	union {
		/// In-line data storage area.
		char bytes[dataSize];

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
	 * The next block must be inside the pool and can not be null.
	 */
	inline void setNext(Block *block) {
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
	 * Get the size of the stored in-line data.
	 */
	inline uint16_t getSize() const {
		return header.sizeAndFlags & static_cast<uint16_t>(~endOfPacket);
	}

	/**
	 * Set the size of the contained data (implies in-line storage).
	 */
	inline void setSize(uint16_t size) {
		header.sizeAndFlags = (header.sizeAndFlags & endOfPacket) | (size & static_cast<uint16_t>(~endOfPacket));
	}

	/**
	 * Check if this block is flagged as the end of a packet.
	 */
	inline bool isEndOfPacket() const {
		return header.sizeAndFlags & endOfPacket;
	}

	/**
	 * Set or reset the end of packet flag.
	 */
	inline void setEndOfPacket(bool x) {
		header.sizeAndFlags = static_cast<uint16_t>(x ? (header.sizeAndFlags | endOfPacket) : (header.sizeAndFlags & ~endOfPacket));
	}

	/**
	 * Get the pointer to the data (in-line or indirect).
	 */
	inline char* getData() {
		return &bytes[0];
	}

	inline char* getIndirectData() {
		return entry.start;
	}

	inline uint16_t getIndirectSize() {
		return entry.size;
	}

	inline void callIndirectDestructor() {
		if(entry.destructor)
			entry.destructor(entry.user, entry.start, entry.size);
	}
};

template<class S, class... Args>
class Network<S, Args...>::Packet
{
	friend Network<S, Args...>;
	Block* first;

public:
	void init(Block* first) {
		this->first = first;
	}

	void dispose() {
		// TODO
	}
};

template<class S, class... Args>
class Network<S, Args...>::PacketAssembler: public Packet
{
protected:
	Block* current;

public:
	inline void addBlock(Block* next) {
		current->setNext(next);
		current = next;
	}

	inline void init(Block* first) {
		Packet::init(first);
		current = first;
	}

	inline void done() {
		current->setEndOfPacket(true);
		current->terminate();
	}
};

template<class S, class... Args>
class Network<S, Args...>::PacketDisassembler
{
protected:
	Block* current;

private:
	bool moveToNextBlock() {
		if(Block* next = current->getNext())
			current = next;
		else
			return false;

		return true;
	}

	inline bool isCurrentIndirect() {
		return current->getSize() == 0x7fff;
	}

public:
	inline void init(const Packet& p) {
		current = p.first;
	}

	inline bool advance() {
		if(current->isEndOfPacket())
			return false;

		return moveToNextBlock();
	}

	inline bool moveToNextPacket() {
		// assert(current->isEndOfPacket());
		return moveToNextBlock();
	}

	inline Chunk getCurrentChunk() {
		char *start, *limit;

		if(isCurrentIndirect()) {
			start = current->getIndirectData();
			limit = start + current->getIndirectSize();
		} else {
			start = current->getData();
			limit = start + current->getSize();
		}

		return Chunk{start, limit};
	}
};

template<class S, class... Args>
struct Network<S, Args...>::PacketTransmissionRequest: Packet, Os::IoJob::Data {
	PacketTransmissionRequest 	*next;

	void init(const Packet& packet) {
		*static_cast<Packet*>(this) = packet;
	}
};

#endif /* PACKET_H_ */
