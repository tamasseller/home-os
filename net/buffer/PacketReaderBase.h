/*
 * PacketReaderBase.h
 *
 *  Created on: 2018.02.13.
 *      Author: tooma
 */

#ifndef PACKETREADERBASE_H_
#define PACKETREADERBASE_H_

#include "Access.h"

template<class S, class... Args>
template <class Child>
struct Network<S, Args...>::PacketReaderBase
{
	uint16_t copyOut(char* output, uint16_t outputLength);

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

	inline bool atEop()
	{
		auto* self = static_cast<Child*>(this);

		return !self->start || (!self->spaceLeft() && self->getCurrentBlock()->isEndOfPacket());
	}

 	inline bool read8(uint8_t &ret)
 	{
 		auto* self = static_cast<Child*>(this);

		if(self->spaceLeft() > 0) {
			ret = *self->start++;
			return true;
		}

		return copyOut(reinterpret_cast<char*>(&ret), 1) == 1;
	}

	inline bool read16raw(uint16_t &ret)
	{
		auto* self = static_cast<Child*>(this);

        if(self->spaceLeft() > 1) {
			ret = home::readUnaligned16(self->start);
			self->start += 2;
	        return true;
        }

        return copyOut(reinterpret_cast<char*>(&ret), 2) == 2;
	}

	inline bool read32raw(uint32_t &ret)
	{
		auto* self = static_cast<Child*>(this);

        if(self->spaceLeft() > 3) {
			ret = home::readUnaligned32(self->start);
			self->start += 4;
			return true;
        }

		return copyOut(reinterpret_cast<char*>(&ret), 4) == 4;
	}


	inline bool read16net(uint16_t &ret)
	{
		if(!read16raw(ret))
			return false;

		ret = home::reverseBytes16(ret);
        return true;
	}

	inline bool read32net(uint32_t &ret)
	{
		if(!read32raw(ret))
			return false;

		ret = home::reverseBytes32(ret);
        return true;
	}
};

template<class S, class... Args>
template <class Child>
uint16_t Network<S, Args...>::PacketReaderBase<Child>::copyOut(char* output, uint16_t outputLength)
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

#endif /* PACKETREADERBASE_H_ */
