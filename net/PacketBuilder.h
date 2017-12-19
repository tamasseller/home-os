/*
 * PacketBuilder.h
 *
 *  Created on: 2017.12.03.
 *      Author: tooma
 */

#ifndef PACKETBUILDER_H_
#define PACKETBUILDER_H_

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
				const uint16_t runLength = (space < leftoverLength) ? space : leftoverLength;
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

	inline bool write32raw(uint32_t data) {
		return copyIn((const char*)&data, 4) == 4; // TODO optimize
	}

	inline bool write16raw(uint16_t data) {
		return copyIn((const char*)&data, 2) == 2; // TODO optimize
	}

	inline bool write8(uint16_t data) {
		return copyIn((const char*)&data, 1) == 1; // TODO optimize
	}

	inline bool write32net(uint32_t data) {
		return write32raw(correctEndian(data));
	}

	inline bool write16net(uint16_t data) {
		return write16raw(correctEndian(data));
	}
};

template<class S, class... Args>
template<typename Network<S, Args...>::Pool::Quota quota>
class Network<S, Args...>::PacketBuilder: public PacketAssembler, public PacketWriterBase<PacketBuilder<quota>>
{
	friend class PacketBuilder::PacketWriterBase;

	static constexpr uint16_t maxLength = static_cast<uint16_t>(Block::dataSize);

	typename Pool::Allocator allocator;
	char *data, *limit;

	constexpr inline uint16_t spaceLeft() const {
		return static_cast<uint16_t>(limit - data);
	}

	inline void finalizeCurrent() {
	    if(data)
	        this->current->setSize(static_cast<uint16_t>(maxLength - spaceLeft()));
	}

	inline bool takeNext() {
		if(Block* next = allocator.get()) {
			finalizeCurrent();
			PacketAssembler::addBlock(next);
			data = next->getData();
			limit = data + maxLength;
			return true;
		}

		return false;
	}

public:
	inline void init(const typename Pool::Allocator& allocator) {
		this->allocator = allocator;
		PacketAssembler::init(this->allocator.get());
		data = this->current->getData();
		limit = data + maxLength;
	}

	inline void done() {
		finalizeCurrent();
		PacketAssembler::done();
		allocator.template freeSpare<quota>(&state.pool);
	}

    inline bool addByReference(const char* data, uint16_t length, typename Block::Destructor destructor = nullptr, void* userData = nullptr)
    {
        Block* block = this->current;

        if(spaceLeft() != maxLength) {
            if(Block* next = allocator.get()) {
                finalizeCurrent();
                PacketAssembler::addBlock(next);
                block = next;
            } else
                return false;
        }

        block->setIndirect(data, length, destructor, userData);
        this->data = this->limit = nullptr;
        return true;
    }

};

#endif /* PACKETBUILDER_H_ */
