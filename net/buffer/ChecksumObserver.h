/*
 * ChecksumObserver.h
 *
 *  Created on: 2018.01.20.
 *      Author: tooma
 */

#ifndef CHECKSUMOBSERVER_H_
#define CHECKSUMOBSERVER_H_

#include "Network.h"

template<class S, class... Args>
template<class Child>
struct Network<S, Args...>::ChecksumObserver: InetChecksumDigester
{
	size_t remainingLength;
	size_t totalLength;
	size_t skip;
	bool offsetOdd;

	inline void updateDigester(Chunk chunk)
	{
		if(chunk.length < remainingLength) {
			this->consume(chunk.start, chunk.length, offsetOdd);
			remainingLength -= chunk.length;
		} else if(remainingLength) {
			this->consume(chunk.start, remainingLength, offsetOdd);
			remainingLength = 0;
		}

		if(chunk.length & 1)
			offsetOdd = !offsetOdd; // TODO rewrite as XOR to motivate that it is compiled like that.
	}

public:
	using InetChecksumDigester::patch;
	using InetChecksumDigester::getReducedState;

	inline void observeInternalBlockLeave() {
		auto chunk = static_cast<Child*>(this)->getFullChunk();
		totalLength += chunk.length;

		chunk.start += skip;
		chunk.length -= skip;

		updateDigester(chunk);
	}

	inline void observeInternalBlockEnter()
	{
	}

	inline void observeFirstBlock()
	{
		uint8_t firstByte = *static_cast<Child*>(this)->data;

		NET_ASSERT((firstByte & 0xf0) == 0x40); // IPv4 only for now.

		remainingLength = (firstByte & 0xf) << 2;
		totalLength = 0;
		offsetOdd = false;
		skip = 0;
	}

	bool finish()
	{
		auto stream = static_cast<Child*>(this);

		if(!stream->skipAhead(static_cast<uint16_t>(remainingLength)))
			return false;

		updateDigester(stream->getFullChunk());
		return true;
	}

	bool finishAndCheck() {
		return finish() && (this->result() == 0);
	}

	bool goToEnd() {
		return !static_cast<Child*>(this)->skipAhead(0xffff);
	}

	size_t getLength() {
		return totalLength;
	}

	bool checkTotalLength(size_t expectedLength) {
		return goToEnd() && (totalLength == expectedLength);
	}

	void restart(size_t newLength)
	{
		InetChecksumDigester::reset();

		remainingLength = newLength;
		offsetOdd = false;

		auto stream = static_cast<Child*>(this);
		auto chunk = static_cast<Child*>(this)->getChunk();
		skip = stream->getFullChunk().length - chunk.length;
	}
};

#endif /* CHECKSUMOBSERVER_H_ */
