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
	public PacketWriterBase<PacketStreamBase<ObserverTemplate>>,
	public PacketReaderBase<PacketStreamBase<ObserverTemplate>>
{
	using Observer = ObserverTemplate<PacketStreamBase<ObserverTemplate>>;
	friend Observer;
	friend class PacketStreamBase::PacketWriterBase;
	friend class PacketStreamBase::PacketReaderBase;

	Block* current;
	char *data, *limit;

	constexpr inline size_t spaceLeft() const {
	    return limit - data;
	}

	inline void updateDataPointers() {
		data = current->getData();
		limit = data + current->getSize();
	}

	inline bool takeNext() {
		if(data)
			static_cast<Observer*>(this)->observeBlockAtLeave();

		if(!current || current->isEndOfPacket() || !current->getNext()) {
			invalidate();
			return false;
		}

		current = current->getNext();
		data = current->getData();
		limit = data + current->getSize();
		return true;
	}

public:
	inline PacketStreamBase() = default;

	inline void init(const Packet& p){
		if((current = Packet::Accessor::getFirst(p)) != nullptr) {
			data = current->getData();
			limit = data + current->getSize();
		} else
			invalidate();
	}

    void invalidate() {
        current = nullptr;
        data = limit = nullptr;
    }

	inline bool isInitialized() {
		return current != nullptr;
	}

	inline Chunk getFullChunk() {
		return Chunk {current->getData(), current->getSize()};
	}
};

#endif /* PACKETSTREAMBASE_H_ */
