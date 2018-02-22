/*
 * PacketStreamState.h
 *
 *  Created on: 2018.02.14.
 *      Author: tooma
 */

#ifndef PACKETSTREAMSTATE_H_
#define PACKETSTREAMSTATE_H_

template<class S, class... Args>
class Network<S, Args...>::ChunkReaderState
{
protected:
	char *start = nullptr, *limit = nullptr;

	constexpr inline size_t spaceLeft() const {
	    return limit - start;
	}

	inline void updateDataPointers(Block* current) {
		start = current ? current->getData() : nullptr;
		limit = current ? (start + current->getSize()) : nullptr;
	}

	inline bool isInitialized() {
		return start != nullptr;
	}
};

template<class S, class... Args>
class Network<S, Args...>::PacketStreamState: protected ChunkReaderState
{
protected:
	Block* current = nullptr;

	inline void updateDataPointers(Block* current) {
		ChunkReaderState::updateDataPointers(this->current = current);
	}

	inline Block* getCurrentBlock() {
		return this->current;
	}
};

#endif /* PACKETSTREAMSTATE_H_ */
