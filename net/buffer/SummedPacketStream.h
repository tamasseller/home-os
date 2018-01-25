/*
 * SummedPacketStream.h
 *
 *  Created on: 2018.01.21.
 *      Author: tooma
 */

#ifndef SUMMEDPACKETSTREAM_H_
#define SUMMEDPACKETSTREAM_H_

#include "Network.h"

template<class S, class... Args>
template<class Child>
struct Network<S, Args...>::ChecksumObserver: InetChecksumDigester
{
	size_t remainingLength;
	size_t totalLength = 0;
	size_t skip = 0;
	bool offsetOdd = false;

	inline size_t getOffset() {
		auto stream = static_cast<Child*>(this);
		return stream->data - stream->current->getData();
	}

	inline void updateDigester(Chunk chunk)
	{
		chunk.start += skip;
		chunk.length -= skip;
		skip = 0;

		if(chunk.length < remainingLength) {
			this->consume(chunk.start, chunk.length, offsetOdd);
			remainingLength -= chunk.length;
		} else if(remainingLength) {
			this->consume(chunk.start, remainingLength, offsetOdd);
			remainingLength = 0;
		}

		offsetOdd ^= (chunk.length & 1);
	}

	inline void observeBlockAtLeave() {
		auto chunk = static_cast<Child*>(this)->getFullChunk();
		totalLength += chunk.length;
		updateDigester(chunk);
	}

public:
	//using InetChecksumDigester::patch;
	using InetChecksumDigester::getReducedState;
	using InetChecksumDigester::result;

	bool finish()
	{
		auto stream = static_cast<Child*>(this);

		if(!stream->skipAhead(static_cast<uint16_t>(remainingLength - getOffset() + skip)))
			return false;

		updateDigester(stream->getFullChunk());

		return true;
	}

	bool goToEnd() {
		return !static_cast<Child*>(this)->skipAhead(0xffff);
	}

	size_t getLength() {
		return totalLength;
	}

	void start(size_t newLength)
	{
		InetChecksumDigester::reset();

		remainingLength = newLength;
		offsetOdd = false;

		auto stream = static_cast<Child*>(this);
		skip += getOffset();
	}

	void patchNet(uint16_t n) {
		this->patch(Network<S, Args...>::correctEndian(n));
	}
};

template<class S, class... Args>
class Network<S, Args...>::SummedPacketStream: public PacketStreamBase<ChecksumObserver>
{
public:
	inline SummedPacketStream() = default;
	inline SummedPacketStream(const Packet& p) {
		this->init(p);
	}
};

#endif /* SUMMEDPACKETSTREAM_H_ */
