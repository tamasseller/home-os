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
	public PacketReaderBase<PacketStreamBase<ObserverTemplate>>,
	public PacketStreamState
{
	using Observer = ObserverTemplate<PacketStreamBase<ObserverTemplate>>;
	friend Observer;
	friend class PacketStreamBase::PacketWriterBase;
	friend class PacketStreamBase::PacketReaderBase;

	inline bool takeNext() {
		if(this->start)
			static_cast<Observer*>(this)->observeBlockAtLeave();

		if(!this->current || this->current->isEndOfPacket() || !this->current->getNext()) {
			this->updateDataPointers(nullptr);
			return false;
		}

		this->updateDataPointers(this->current->getNext());
		return true;
	}

public:
	inline PacketStreamBase() = default;

    // Internal!
    inline bool onBlocking()
    {
        return this->isEmpty();
    }

	inline void init(const Packet& p){
		this->updateDataPointers(Packet::Accessor::getFirst(p));
	}

	inline void invalidate() {
		this->updateDataPointers(nullptr);
	}

	inline Chunk getFullChunk() {
		return Chunk {this->current->getData(), this->current->getSize()};
	}
};

#endif /* PACKETSTREAMBASE_H_ */
