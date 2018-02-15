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
class Network<S, Args...>::PacketBuilder: public PacketAssembler, public PacketWriterBase<PacketBuilder>
{
	friend class PacketBuilder::PacketWriterBase;

	static constexpr uint16_t maxLength = static_cast<uint16_t>(Block::dataSize);

	typename Pool::Allocator allocator;
	char *start, *limit;

	constexpr inline uint16_t spaceLeft() const {
		return static_cast<uint16_t>(limit - start);
	}

	inline void finalizeCurrent() {
	    if(start)
	        this->current->setSize(static_cast<uint16_t>(maxLength - spaceLeft()));
	}

	inline bool takeNext() {
		if(Block* next = allocator.get()) {
			finalizeCurrent();
			PacketAssembler::addBlock(next);
			start = next->getData();
			limit = start + maxLength;
			return true;
		}

		return false;
	}

public:
	inline PacketBuilder() = default;
	inline PacketBuilder(const typename Pool::Allocator& allocator) {
		this->init(allocator);
	}

	inline void init(const typename Pool::Allocator& allocator) {
		this->allocator = allocator;
		PacketAssembler::init(this->allocator.get());
		start = this->current->getData();
		limit = start + maxLength;
	}

	inline void done() {
		finalizeCurrent();
		PacketAssembler::done();
		allocator.template freeSpare<Pool::Quota::Tx>(&state.pool);
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

        block->setExternal(data, length, destructor, userData);
        this->start = this->limit = nullptr;
        return true;
    }

};

#endif /* PACKETBUILDER_H_ */
