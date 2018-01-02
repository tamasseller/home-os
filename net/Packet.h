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
	typedef void (*Destructor)(void*, const char*, uint16_t);

private:
    friend Network<S, Args...>::Packet;

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
		/**
		 * Packet size, offset and end of packet flags.
		 *
		 * | EOP:1 | offset:2 | size: 13 |
		 *
		 * Special value:
		 *  - If the value of the offset field is maximal then the actual offset is contained in the
		 *    the first two bytes of in-line data area, otherwise it is the offset value itself.
		 *  - If the size field is maximal then the packet is indirect (and can not have an offset).
		 */
		uint16_t sizeAndFlags = 0;
	};

	struct IndirectEntry {
        Destructor destructor;
        void *user;
        const char *start;
		uint16_t size;
	};

public:
	/*
	 * Constant definitions
	 */
	static constexpr size_t dataSize = bufferSize - sizeof(Header);

private:
	static constexpr uint16_t endOfPacket = 0x8000;
	static constexpr uint16_t offsetMask = 0x6000;
	static constexpr size_t offsetShift = 13;
	static constexpr uint16_t sizeMask = 0x1fff;

	static_assert(dataSize >= sizeof(IndirectEntry), "Pool blocks are too small");
    static_assert(bufferSize < Block::sizeMask, "Pool blocks are too big");


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

	static constexpr size_t dataOffset() {
		return reinterpret_cast<size_t>(reinterpret_cast<Block*>(0)->bytes);
	}

    /**
     * Set the size field in _sizeAndFlags_.
     */
    inline void setSizeInternal(uint16_t size) {
        header.sizeAndFlags = static_cast<uint16_t>(header.sizeAndFlags & ~sizeMask) | size;
    }

    /**
     * Get offset of usable data inside the in-line data area.
     */
    inline void writeOffset(uint16_t newOffset) {
        if(newOffset < (offsetMask >> offsetShift))
            header.sizeAndFlags = static_cast<uint16_t>((header.sizeAndFlags & ~offsetMask) | (newOffset << offsetShift));
        else {
            header.sizeAndFlags = static_cast<uint16_t>(header.sizeAndFlags | offsetMask);
            bytes[0] = static_cast<uint8_t>(newOffset);
            bytes[1] = static_cast<uint8_t>(newOffset >> 8);
        }
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
		return header.sizeAndFlags & sizeMask;
	}

    /**
     * Get offset of usable data inside the in-line data area.
     */
    inline uint16_t getOffset() const {
        auto maskedOffset = (header.sizeAndFlags & offsetMask);

        if(!maskedOffset)
            return 0;

        Os::assert(!isIndirect(), NetErrorStrings::unknown);

        auto immediateOffset = maskedOffset >> offsetShift;

        if(immediateOffset < 3)
            return static_cast<uint16_t>(immediateOffset);

        return static_cast<uint16_t>(bytes[0] | bytes[1] << 8);
    }

	/**
	 * Set the size of the contained data (implies in-line storage).
	 */
	inline void setSize(uint16_t size) {
	    Os::assert(size < sizeMask, NetErrorStrings::unknown);
		setSizeInternal(size);
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
		return &bytes[0] + getOffset();
	}

	inline const char* getIndirectData() {
		return entry.start;
	}

	inline uint16_t getIndirectSize() {
		return entry.size;
	}

	inline void setIndirect(const char* data, uint16_t length, typename Block::Destructor destructor, void* userData) {
	    entry.start = data;
	    entry.size = length;
	    entry.destructor = destructor;
	    entry.user = userData;
	    setSizeInternal(sizeMask);
	}

    inline bool isIndirect() const {
        return getSize() == sizeMask;
    }

	inline void callIndirectDestructor() {
		if(entry.destructor)
			entry.destructor(entry.user, entry.start, entry.size);
	}

	static inline Block* fromInlineData(char* data) {
		return reinterpret_cast<Block*>(data - dataOffset());
	}
};

template<class S, class... Args>
class Network<S, Args...>::Packet
{
	friend Network<S, Args...>::PacketQueue;
	friend Network<S, Args...>::PacketProcessor;
	friend Network<S, Args...>::PacketDisassembler;
	Block* first;

public:
	void init(Block* first) {
		this->first = first;
	}

	bool isValid() {
		return this->first != nullptr;
	}

	template<typename Pool::Quota quota>
	void dispose() {
        typename Pool::Deallocator deallocator(first);

        for(Block* current = first->getNext(); current;) {
            Block* next = current->getNext();

            if(current->isIndirect())
                current->callIndirectDestructor();

            if(current->isEndOfPacket()) {
                deallocator.take(current);
                break;
            } else
                deallocator.take(current);

            current = next;
        }

        deallocator.template deallocate<quota>(&state.pool);
	}

    /**
     * Increase data offset at the start of the packet.
     *
     * If block becomes empty it is dropped.
     */
	template<typename Pool::Quota quota>
    inline void dropInitialBytes(size_t n) {
        Os::assert(!first->isIndirect(), NetErrorStrings::unknown);

        while(n) {
            if(n < first->getSize()) {
                first->writeOffset(static_cast<uint16_t>(first->getOffset() + n));
                first->setSize(static_cast<uint16_t>(first->getSize() - n));
                break;
            } else {
                Block* current = first->getNext();
                n -= first->getSize();

                typename Pool::Deallocator deallocator(first);

                while(current && (n >= current->getSize())) {
                    Os::assert(!current->isIndirect(), NetErrorStrings::unknown);

                    Block* next = current->getNext();

                    n -= first->getSize();
                    deallocator.take(first);

                    current = next;
                }

                deallocator.template deallocate<quota>(&state.pool);

                if((first = current) == nullptr)
                    break;
            }
        }
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

	inline void addBlockByFinalInlineData(char* data, uint16_t length) {
		auto next = Block::fromInlineData(data);
		next->setSize(length);
		current->setNext(next);
		current = next;
	}

	inline void init(Block* first) {
		Packet::init(first);
		current = first;
	}

	inline void initByFinalInlineData(char* data, uint16_t length) {
		auto first = Block::fromInlineData(data);
		first->setSize(length);
		init(first);
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

	bool moveToNextBlock() {
		if(Block* next = current->getNext())
			current = next;
		else
			return false;

		return true;
	}

public:
	inline void init(const Packet& p) {
		current = p.first;
	}

	inline Packet asPacket() {
	    Packet ret;
	    ret.first = current;
	    return ret;
	}

	inline bool advance() {
		if(!current || current->isEndOfPacket())
			return false;

		return moveToNextBlock();
	}

	inline Chunk getCurrentChunk() {
		char *start, *limit;

		if(current->isIndirect()) {
			start = const_cast<char*>(current->getIndirectData());
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
	PacketTransmissionRequest *next;

	void init(const Packet& packet) {
		*static_cast<Packet*>(this) = packet;
	}
};

#endif /* PACKET_H_ */
