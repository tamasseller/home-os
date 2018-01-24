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
class Network<S, Args...>::PacketWriterBase {
public:
	uint16_t copyIn(const char* input, uint16_t inputLength)
	{
		auto* self = static_cast<Child*>(this);

		uint16_t done = 0;

		while(const uint16_t leftoverLength = static_cast<uint16_t>(inputLength - done)) {
			if(const auto space = self->spaceLeft()) {
				const size_t runLength = (space < leftoverLength) ? space : leftoverLength;
				memcpy(self->data, input, runLength);
				done = static_cast<uint16_t>(done + runLength);
				input += runLength;
				self->data += runLength;
			} else {
				if(!self->takeNext())
					break;
			}
		}

		return done;
	}

	uint16_t copyFrom(PacketStream& stream, uint16_t inputLength)
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

    inline bool write8(uint16_t data) {
        return copyIn((const char*)&data, 1) == 1; // TODO optimize
    }

	inline bool write32raw(uint32_t data) {
	    // TODO move net/native order unaligned memory access abstraction to OS platform dependent module.
		return copyIn((const char*)&data, 4) == 4; // TODO optimize
	}

	inline bool write16raw(uint16_t data) {
	    // TODO move net/native order unaligned memory access abstraction to OS platform dependent module.
		return copyIn((const char*)&data, 2) == 2; // TODO optimize
	}

	inline bool write32net(uint32_t data) {
	    // TODO move net/native order unaligned memory access abstraction to OS platform dependent module.
		return write32raw(correctEndian(data));
	}

	inline bool write16net(uint16_t data) {
	    // TODO move net/native order unaligned memory access abstraction to OS platform dependent module.
		return write16raw(correctEndian(data));
	}
};

#endif /* PACKETWRITERBASE_H_ */
