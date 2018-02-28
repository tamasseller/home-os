/*
 * PacketWriterBase.h
 *
 *  Created on: 2018.01.21.
 *      Author: tooma
 */

#ifndef PACKETWRITERBASE_H_
#define PACKETWRITERBASE_H_

#include "Network.h"

template<class S, class... Args>
template <class Child>
struct Network<S, Args...>::PacketWriterBase
{
	uint16_t copyIn(const char* input, uint16_t inputLength);
	uint16_t copyFrom(PacketStream& stream, uint16_t inputLength);

    bool skipAhead(uint16_t length)
    {
        auto* self = static_cast<Child*>(this);

        while (length) {
            if(self->spaceLeft() >= length) {
                self->start += length;
                break;
            } else {
                length = static_cast<uint16_t>(length - self->spaceLeft());

                if(!self->takeNext())
                    return false;
            }
        }

        return true;
    }

    inline bool write8(uint8_t data) {
 		auto* self = static_cast<Child*>(this);

		if(self->spaceLeft() > 0) {
			*self->start++ = data;
			return true;
		}

		return copyIn((const char*)&data, 1) == 1;
    }

	inline bool write16raw(uint16_t data) {
		auto* self = static_cast<Child*>(this);

        if(self->spaceLeft() > 1) {
			home::writeUnaligned16(self->start, data);
			self->start += 2;
	        return true;
        }

		return copyIn((const char*)&data, 2) == 2;
	}

	inline bool write32raw(uint32_t data) {
		auto* self = static_cast<Child*>(this);

        if(self->spaceLeft() > 3) {
			home::writeUnaligned32(self->start, data);
			self->start += 4;
			return true;
        }

		return copyIn((const char*)&data, 4) == 4;
	}

	inline bool write16net(uint16_t data) {
	    return write16raw(home::reverseBytes16(data));
	}

	inline bool write32net(uint32_t data) {
	    return write32raw(home::reverseBytes32(data));
	}
};

template<class S, class... Args>
template <class Child>
uint16_t Network<S, Args...>::PacketWriterBase<Child>::copyIn(const char* input, uint16_t inputLength)
{
	auto* self = static_cast<Child*>(this);

	uint16_t done = 0;

	while(const uint16_t leftoverLength = static_cast<uint16_t>(inputLength - done)) {
		if(const auto space = self->spaceLeft()) {
			const size_t runLength = (space < leftoverLength) ? space : leftoverLength;
			memcpy(self->start, input, runLength);
			done = static_cast<uint16_t>(done + runLength);
			input += runLength;
			self->start += runLength;
		} else {
			if(!self->takeNext())
				break;
		}
	}

	return done;
}

template<class S, class... Args>
template <class Child>
uint16_t Network<S, Args...>::PacketWriterBase<Child>::copyFrom(PacketStream& stream, uint16_t inputLength)
{
    auto* self = static_cast<Child*>(this);

    uint16_t done = 0;

    while(inputLength) {
        Chunk chunk = stream.getChunk();

        if(!chunk.length)
            break;

        uint16_t run = inputLength;

        if(run > chunk.length)
            run = static_cast<uint16_t>(chunk.length);

        auto actualRunLength = this->copyIn(chunk.start, run);

        if(actualRunLength != run)
            run = inputLength = actualRunLength;

        stream.advance(run);
        inputLength = static_cast<uint16_t>(inputLength - run);
        done = static_cast<uint16_t>(done + run);
    }

    return done;
}

#endif /* PACKETWRITERBASE_H_ */
