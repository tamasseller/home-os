/*
 * PacketReaderBase.h
 *
 *  Created on: 2018.02.13.
 *      Author: tooma
 */

#ifndef PACKETREADERBASE_H_
#define PACKETREADERBASE_H_

template<class S, class... Args>
template <class Child>
struct Network<S, Args...>::PacketReaderBase
{
	inline Chunk getChunk()
	{
		auto* self = static_cast<Child*>(this);

		if(!self->spaceLeft()) {
			if(!self->takeNext())
				return Chunk {nullptr, 0};
		}

		return Chunk {self->start, self->spaceLeft()};
	}

	inline void advance(size_t x)
	{
		auto* self = static_cast<Child*>(this);
		self->start += x;
	}

	inline uint16_t copyOut(char* output, uint16_t outputLength)
	{
		auto* self = static_cast<Child*>(this);

		uint16_t done = 0;
		while(const uint16_t leftoverLength = static_cast<uint16_t>(outputLength - done)) {
			if(const auto space = self->spaceLeft()) {
				const size_t runLength = (space < leftoverLength) ? space : leftoverLength;
				memcpy(output, self->start, runLength);
				done = static_cast<uint16_t>(done + runLength);
				output += runLength;
				self->start += runLength;
			} else {
				if(!self->takeNext())
					break;
			}
		}

		return done;
	}

	inline bool atEop()
	{
		auto* self = static_cast<Child*>(this);

		return !self->start || (!self->spaceLeft() && self->current->isEndOfPacket());
	}

 	inline bool read8(uint8_t &ret)
 	{
 		auto* self = static_cast<Child*>(this);

		if(self->spaceLeft() > 0) {
			ret = *self->start++;
			return true;
		} else
			return copyOut(reinterpret_cast<char*>(&ret), 1) == 1;
	}

	inline bool read16net(uint16_t &ret)
	{
		auto* self = static_cast<Child*>(this);

		if(self->spaceLeft() > 1) {
			uint16_t b1 = static_cast<uint8_t>(*self->start++);                    // TODO move net/native order unaligned memory access abstraction to OS platform dependent module.
			uint16_t b2 = static_cast<uint8_t>(*self->start++);
			ret = static_cast<uint16_t>(b1 << 8 | b2);
		} else {
			if(copyOut(reinterpret_cast<char*>(&ret), 2) != 2)
			    return false;

			ret = correctEndian(ret);
		}

        return true;
	}

	inline bool read32net(uint32_t &ret)
	{
		auto* self = static_cast<Child*>(this);

		if(self->spaceLeft() > 3) {
			uint32_t b1 = static_cast<uint8_t>(*self->start++);                    // TODO move net/native order unaligned memory access abstraction to OS platform dependent module.
			uint32_t b2 = static_cast<uint8_t>(*self->start++);
			uint32_t b3 = static_cast<uint8_t>(*self->start++);
			uint32_t b4 = static_cast<uint8_t>(*self->start++);
			ret = b1 << 24 | b2 << 16 | b3 << 8 | b4;
		} else {
			if(copyOut(reinterpret_cast<char*>(&ret), 4) != 4)
			    return false;

			ret = correctEndian(ret);
		}

		return true;
	}

	inline bool read16raw(uint16_t &ret)
	{
		auto* self = static_cast<Child*>(this);

        if(self->spaceLeft() > 1) {
            uint16_t b1 = static_cast<uint8_t>(*self->start++);                    // TODO move net/native order unaligned memory access abstraction to OS platform dependent module.
            uint16_t b2 = static_cast<uint8_t>(*self->start++);
            ret = correctEndian(static_cast<uint16_t>(b1 << 8 | b2));
        } else {
            if(copyOut(reinterpret_cast<char*>(&ret), 2) != 2)
                return false;
        }

        return true;
	}

	inline bool read32raw(uint32_t &ret)
	{
		auto* self = static_cast<Child*>(this);

        if(self->spaceLeft() > 3) {
            uint32_t b1 = static_cast<uint8_t>(*self->start++);                    // TODO move net/native order unaligned memory access abstraction to OS platform dependent module.
            uint32_t b2 = static_cast<uint8_t>(*self->start++);
            uint32_t b3 = static_cast<uint8_t>(*self->start++);
            uint32_t b4 = static_cast<uint8_t>(*self->start++);
            ret = correctEndian(b1 << 24 | b2 << 16 | b3 << 8 | b4);
        } else {
            if(copyOut(reinterpret_cast<char*>(&ret), 4) != 4)
                return false;
        }

        return true;
	}
};

#endif /* PACKETREADERBASE_H_ */
