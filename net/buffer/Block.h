/*
 * Block.h
 *
 *  Created on: 2018.01.20.
 *      Author: tooma
 */

#ifndef BLOCK_H_
#define BLOCK_H_

#include "Network.h"

/**
 * Basic block if packet storage.
 *
 * All packets handled by the system are composed of these _Block_ object allocated
 * from the block pool. It can contain data in itself or it can reference external
 * data that was not allocated from the pool. It also supports arbitrary destructor
 * for these data (with an additional user defined pointer sized argument for it).
 * It has a data area and highly compact header that allows for:
 *
 *  - Linking them into chains to form packets, multi-packet chains or stream data;
 *  - Identifying the last block of a packet (useful in the multi-packet scenario);
 *  - Storing the (mutable) start of data pointer in the (possibly external) data area;
 *  - Storing the length of the data from the starting point in the data area;
 *  - Identifying if the block is used for external data, in which case the normal,
 *    in-line data area contains the extended information regarding the external data.
 */
template<class S, class... Args>
class Network<S, Args...>::Block {
public:
	/**
	 * Signature for thi optional destructor for external blocks.
	 *
	 * @param user The arbitrary user defined argument.
	 * @param data The pointer to the start of the data block (as initialized).
	 * @param length The length of the data block (as initialized).
	 */
	typedef void (*Destructor)(void* user, const char* data, uint32_t length);

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
    	static constexpr uint16_t endOfPacket = 0x8000;
    	static constexpr uint16_t offsetMask = 0x6000;
    	static constexpr size_t offsetShift = 13;
    	static constexpr uint16_t sizeMask = 0x1fff;

		int16_t nextOffset = 0;
		/**
		 * Packet size, offset and end of packet flags.
		 *
		 * | EOP:1 | offset:2 | size: 13 |
		 *
		 * Special value:
		 *  - If the value of the offset field is maximal then the actual offset is contained in the
		 *    the first two bytes of in-line data area, otherwise it is the offset value itself.
		 *  - If the size field is maximal then the block has external data.
		 */
		uint16_t sizeAndFlags = 0;
	};

	/**
	 * External data information.
	 *
	 * Extended data stored in the in-line area to be used for external data.
	 */
	struct External {
		/// The user defined destructor, called on cleanup.
        Destructor destructor;

        /// The user defined arbitrary parameter, passed to the destructor.
        void *user;

        /// The start of the external data (stored as is, not modified for offsetting).
        const char *start;

        /// The size of the external data (stored as is, not modified for offsetting).
		uint32_t size;

		/// The offset from the original start of the external data.
		uint32_t offset;
	};

public:
	/*
	 * Constant definitions
	 */
	static constexpr size_t dataSize = bufferSize - sizeof(Header);

private:
	static_assert(dataSize >= sizeof(External), "Pool blocks are too small");
    static_assert(bufferSize < Block::Header::sizeMask, "Pool blocks are too big");

    /**
     * Header field storage.
     *
     * See notes on the type definition.
     */
	Header header;

    /**
     * Dual-role data storage.
     */
	union {
		/// In-line data storage area.
		char bytes[dataSize] alignas (uint16_t);

		/// Extended info storage for external data (with optional destructor).
		External external;
	};

	/// Helper to get the offset of the data field on more-or-less standard way.
	static constexpr size_t dataOffset() {
		return reinterpret_cast<size_t>(reinterpret_cast<Block*>(0)->bytes);
	}

    /**
     * Set the size field in _sizeAndFlags_.
     */
    inline void writeSizeField(uint16_t size) {
        header.sizeAndFlags = static_cast<uint16_t>(header.sizeAndFlags & ~Header::sizeMask) | size;
    }

    /**
	 * Get the size of the stored in-line data.
	 */
	inline uint16_t readSizeField() const {
		return header.sizeAndFlags & Header::sizeMask;
	}

    /**
     * set offset of usable data inside the in-line data area.
     */
    inline void writeInlineOffset(uint16_t newOffset) {
        if(newOffset < (Header::offsetMask >> Header::offsetShift)) {
            header.sizeAndFlags = static_cast<uint16_t>(
            		(header.sizeAndFlags & ~Header::offsetMask) | (newOffset << Header::offsetShift));
        } else {
            header.sizeAndFlags = static_cast<uint16_t>(header.sizeAndFlags | Header::offsetMask);
            *reinterpret_cast<uint16_t*>(bytes) = newOffset;
        }
    }

    /**
     * Get offset of usable data inside the in-line data area.
     */
    inline uint16_t getInlineOffset() const {
        auto maskedOffset = (header.sizeAndFlags & Header::offsetMask);

        if(!maskedOffset)
            return 0;

        NET_ASSERT(!isExternal());

        auto immediateOffset = maskedOffset >> Header::offsetShift;

        if(immediateOffset < 3)
            return static_cast<uint16_t>(immediateOffset);

        return *reinterpret_cast<const uint16_t*>(bytes);
    }

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
		NET_ASSERT(block != nullptr);
		header.nextOffset = static_cast<uint16_t>(block - this);
	}

	/**
	 * Terminate chain of blocks.
	 *
	 * Sets the block in such a state that it indicates that there is no next block.
	 */
	inline void terminate() {
		header.nextOffset = 0;
	}

	/**
	 * Get the size of the stored data.
	 */
	inline uint32_t getSize() const {
		if(isExternal())
			return external.size - external.offset;
		else
			return readSizeField();
	}

	/**
	 * Set the size of the contained data (implies in-line storage).
	 */
	inline void setSize(uint16_t size) {
	    NET_ASSERT(size < Header::sizeMask);
		writeSizeField(size);
	}

	/**
	 * Check if this block is flagged as the end of a packet.
	 */
	inline bool isEndOfPacket() const {
		return header.sizeAndFlags & Header::endOfPacket;
	}

	/**
	 * Set the end of packet flag.
	 */
	inline void setEndOfPacket(bool x) {
		header.sizeAndFlags = static_cast<uint16_t>(header.sizeAndFlags | Header::endOfPacket);
	}

	/**
	 * Reset the end of packet flag.
	 */
	inline void clearEndOfPacket(bool x) {
		header.sizeAndFlags = static_cast<uint16_t>(header.sizeAndFlags & ~Header::endOfPacket);
	}

	/**
	 * Get the pointer to the data (in-line or external).
	 */
	inline char* getData() {
		if(isExternal())
			return const_cast<char*>(external.start + external.offset);
		else
			return &bytes[0] + getInlineOffset();
	}

	/**
	 * Initialize the block as indirect, using the user supplied information.
	 */
	inline void setExternal(const char* data, uint32_t length, typename Block::Destructor destructor, void* userData) {
	    external.start = data;
	    external.size = length;
	    external.destructor = destructor;
	    external.user = userData;
	    external.offset = 0;
	    writeSizeField(Header::sizeMask);
	}

	/**
	 * Return whether the current block is indirect.
	 */
	inline bool isExternal() const {
        return readSizeField() == Header::sizeMask;
    }

	/**
	 * Increase the data offset inside the (external or internal) data.
	 */
	inline void increaseOffset(size_t n) {
		if(isExternal()) {
			external.offset = static_cast<uint32_t>(external.offset + n);
		} else {
			NET_ASSERT(n <= getSize());
		    writeInlineOffset(static_cast<uint16_t>(getInlineOffset() + n));
		    setSize(static_cast<uint16_t>(getSize() - n));
		}
	}

	/**
	 * Do the necessary cleanup before disposal.
	 *
	 * Calls the destructor for blocks that have external data and a destructor specified.
	 */
	inline void cleanup() {
		if(isExternal() && external.destructor)
			external.destructor(external.user, external.start, external.size);
	}

	/**
	 * Magic cast from data in-line data pointer.
	 *
	 * Returns the pointer to the _Block_ object that has the in-line data area
	 * provided using a semi-standard, unchecked method.
	 *
	 * @NOTE It should only be called on the actual start of the data area, it has no way
	 *       to handle a nonzero offset (that would require to know the block in advance).
	 */
	static inline Block* fromInlineData(char* data) {
		return reinterpret_cast<Block*>(data - dataOffset());
	}

	/**
	 * Get the last block of a packet.
	 *
	 * Goes along the chain until an end-of-packet flagged block is found.
	 *
	 * @NOTE It should only be called on chains of blocks that represent and actual packet, and
	 *       thus have a last block that is flagged as end-of-packet.
	 */
	inline Block* findEndOfPacket()
	{
		Block* block = this;

		while(!block->isEndOfPacket()) {
			block = block->getNext();
			NET_ASSERT(block != nullptr);
		}

		return block;
	}

};

#endif /* BLOCK_H_ */
