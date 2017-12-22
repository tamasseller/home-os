/*
 * PacketStream.h
 *
 *  Created on: 2017.12.03.
 *      Author: tooma
 */

#ifndef PACKETSTREAM_H_
#define PACKETSTREAM_H_

template<class S, class... Args>
template<class Observer>
class Network<S, Args...>::ObservedPacketStream:
	public Observer,
	public PacketDisassembler,
	public PacketWriterBase<PacketStream>
{
	friend class ObservedPacketStream::PacketWriterBase;

	char *data, *limit;

	constexpr inline uint16_t spaceLeft() const {
		return static_cast<uint16_t>(limit - data);
	}

	inline bool takeNext() {
		if(!PacketDisassembler::advance())
			return false;

		Chunk ret = this->getCurrentChunk();
		data = ret.start;
		limit = ret.end;

		static_cast<Observer*>(this)->observeInternalBlock(data, limit);
		return true;
	}

public:
	inline void init(const Packet& p){
		PacketDisassembler::init(p);
		/*
		 * Any packet must have headers that can only be written to in-line blocks,
		 * so it is safe to assume that the first block contains in-line data.
		 */
		data = this->current->getData();
		limit = data + this->current->getSize();
		static_cast<Observer*>(this)->observeFirstBlock(data, limit);
	}

	inline Chunk getChunk()
	{
		if(!spaceLeft()) {
			if(!takeNext())
				return Chunk {nullptr, nullptr};
		}

		return Chunk {data, limit};
	}

	inline void advance(uint16_t x) {
		data += x;
	}

	inline uint16_t copyOut(char* output, uint16_t outputLength) {
		uint16_t done = 0;
		while(const uint16_t leftoverLength = static_cast<uint16_t>(outputLength - done)) {
			if(const auto space = spaceLeft()) {
				const uint16_t runLength = (space < leftoverLength) ? space : leftoverLength;
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

    inline bool cutCurrentAndMoveToNext() {
		while(!this->current->isEndOfPacket()) {
			if(!this->moveToNextBlock())
				return false; // This line is not intended to be reached (LCOV_EXCL_LINE).

			static_cast<Observer*>(this)->observeInternalBlock(
					this->current->getData(), this->current->getData() + this->current->getSize());
		}

		Block* next = this->current->getNext();
		this->current->terminate();

		if((this->current = next) == nullptr)
			return false;

        data = this->current->getData();
        limit = data + this->current->getSize();
        static_cast<Observer*>(this)->observeFirstBlock(data, limit);
        return true;
    }

	inline bool atEop() {
		return !spaceLeft() && this->current->isEndOfPacket();
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

template<class S, class... Args>
struct Network<S, Args...>::NullObserver {
	inline void observeFirstBlock(char*, char*) {}
	inline void observeInternalBlock(char*, char*) {}
};

template<class S, class... Args>
class Network<S, Args...>::PacketStream: public ObservedPacketStream<NullObserver> {};



#endif /* PACKETSTREAM_H_ */
