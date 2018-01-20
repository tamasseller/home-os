/*
 * PacketStreamBase.h
 *
 *  Created on: 2018.01.20.
 *      Author: tooma
 */

#ifndef PACKETSTREAMBASE_H_
#define PACKETSTREAMBASE_H_

template<class S, class... Args>
struct Network<S, Args...>::Chunk {
	char *start;
	size_t length;
};

template<class S, class... Args>
template<template<class> class ObserverTemplate>
class Network<S, Args...>::PacketStreamBase:
	public ObserverTemplate<PacketStreamBase<ObserverTemplate>>,
	public PacketWriterBase<PacketStreamBase<ObserverTemplate>>
{
	using Observer = ObserverTemplate<PacketStreamBase<ObserverTemplate>>;
	friend Observer;
	friend class PacketStreamBase::PacketWriterBase;

	Block* current;
	char *data, *limit;

	constexpr inline size_t spaceLeft() const {
	    return limit - data;
	}

	inline void updateDataPointers() {
		data = current->getData();
		limit = data + current->getSize();
	}

	inline void updateDataPointers(size_t offset) {
		data = current->getData() + offset;
		limit = data + current->getSize() - offset;
	}


	inline bool advance() {
		if(!current || current->isEndOfPacket())
			return false;

		if(Block* next = current->getNext())
			current = next;
		else
			return false;

		return true;
	}

	inline bool takeNext() {
		static_cast<Observer*>(this)->observeInternalBlock();

		if(!advance())
			return false;

		updateDataPointers();
		return true;
	}


public:
	inline PacketStreamBase() = default;

	inline void init(const Packet& p){
		current = Packet::Accessor::getFirst(p);
		updateDataPointers();
		static_cast<Observer*>(this)->observeFirstBlock();
	}

	inline void init(const Packet& p, size_t offset)
	{
		current = Packet::Accessor::getFirst(p);

		while(offset > current->getSize()) {
			offset -= current->getSize();
			if(!advance()) {
				invalidate();
				return;
			}
		}

		updateDataPointers(offset);
		static_cast<Observer*>(this)->observeFirstBlock();
	}

    void invalidate() {
        current = nullptr;
        data = limit = nullptr;
    }

	inline Chunk getChunk()
	{
		if(!spaceLeft()) {
			if(!takeNext())
				return Chunk {nullptr, 0};
		}

		return Chunk {data, spaceLeft()};
	}

	inline void advance(size_t x) {
		data += x;
	}

	inline uint16_t copyOut(char* output, uint16_t outputLength) {
		uint16_t done = 0;
		while(const uint16_t leftoverLength = static_cast<uint16_t>(outputLength - done)) {
			if(const auto space = spaceLeft()) {
				const size_t runLength = (space < leftoverLength) ? space : leftoverLength;
				memcpy(output, data, runLength);
				done = static_cast<uint16_t>(done + runLength);
				output += runLength;
				data += runLength;
			} else {
				if(!takeNext())
					break;
			}
		}

		return done;
	}

	inline bool skipAhead(uint16_t length)
	{
		while (length) {
			if(spaceLeft() >= length) {
				data += length;
				break;
			} else {
				length = static_cast<uint16_t>(length - spaceLeft());

				if(!takeNext())
					return false;
			}
		}

		return true;
	}

	inline bool atEop() {
		return !data || (!spaceLeft() && current->isEndOfPacket());
	}

 	inline bool read8(uint8_t &ret) {
		if(spaceLeft() > 0) {
			ret = *data++;
			return true;
		} else
			return copyOut(reinterpret_cast<char*>(&ret), 1) == 1;
	}

	inline bool read16net(uint16_t &ret)
	{
		if(spaceLeft() > 1) {
			uint16_t b1 = static_cast<uint8_t>(*data++);                    // TODO move net/native order unaligned memory access abstraction to OS platform dependent module.
			uint16_t b2 = static_cast<uint8_t>(*data++);
			ret = static_cast<uint16_t>(b1 << 8 | b2);
		} else {
			if(copyOut(reinterpret_cast<char*>(&ret), 2) != 2)
			    return false;

			ret = correctEndian(ret);
		}

        return true;
	}

	inline bool read32net(uint32_t &ret) {
		if(spaceLeft() > 3) {
			uint32_t b1 = static_cast<uint8_t>(*data++);                    // TODO move net/native order unaligned memory access abstraction to OS platform dependent module.
			uint32_t b2 = static_cast<uint8_t>(*data++);
			uint32_t b3 = static_cast<uint8_t>(*data++);
			uint32_t b4 = static_cast<uint8_t>(*data++);
			ret = b1 << 24 | b2 << 16 | b3 << 8 | b4;
		} else {
			if(copyOut(reinterpret_cast<char*>(&ret), 4) != 4)
			    return false;

			ret = correctEndian(ret);
		}

		return true;
	}

	inline bool read16raw(uint16_t &ret) {
        if(spaceLeft() > 1) {
            uint16_t b1 = static_cast<uint8_t>(*data++);                    // TODO move net/native order unaligned memory access abstraction to OS platform dependent module.
            uint16_t b2 = static_cast<uint8_t>(*data++);
            ret = correctEndian(static_cast<uint16_t>(b1 << 8 | b2));
        } else {
            if(copyOut(reinterpret_cast<char*>(&ret), 2) != 2)
                return false;
        }

        return true;
	}

	inline bool read32raw(uint32_t &ret) {
        if(spaceLeft() > 3) {
            uint32_t b1 = static_cast<uint8_t>(*data++);                    // TODO move net/native order unaligned memory access abstraction to OS platform dependent module.
            uint32_t b2 = static_cast<uint8_t>(*data++);
            uint32_t b3 = static_cast<uint8_t>(*data++);
            uint32_t b4 = static_cast<uint8_t>(*data++);
            ret = correctEndian(b1 << 24 | b2 << 16 | b3 << 8 | b4);
        } else {
            if(copyOut(reinterpret_cast<char*>(&ret), 4) != 4)
                return false;
        }

        return true;
	}
};

#endif /* PACKETSTREAMBASE_H_ */
