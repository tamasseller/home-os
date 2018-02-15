/*
 * PacketStreamState.h
 *
 *  Created on: 2018.02.14.
 *      Author: tooma
 */

#ifndef PACKETSTREAMSTATE_H_
#define PACKETSTREAMSTATE_H_

template<class S, class... Args>
class Network<S, Args...>::PacketStreamState {
protected:
	Block* current = nullptr; // TODO eliminate
	char *start = nullptr, *limit = nullptr;

	constexpr inline size_t spaceLeft() const {
	    return limit - start;
	}

	inline void updateDataPointers(Block* current) {
		this->current = current;
		start = current ? current->getData() : nullptr;
		limit = current ? (start + current->getSize()) : nullptr;
	}

    inline void invalidate() { // TODO eliminate
        current = nullptr;
        start = limit = nullptr;
    }

	inline bool isInitialized() {
		return current != nullptr;
	}
};




#endif /* PACKETSTREAMSTATE_H_ */
