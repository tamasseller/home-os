/*
 * ValidatorPacketStream.h
 *
 *  Created on: 2018.01.20.
 *      Author: tooma
 */

#ifndef VALIDATORPACKETSTREAM_H_
#define VALIDATORPACKETSTREAM_H_

template<class S, class... Args>
template<class Child>
struct Network<S, Args...>::ChecksumValidatorObserver: InetChecksumDigester {
    size_t remainingLength;
    size_t totalLength;
    bool offsetOdd;

public:
    using InetChecksumDigester::patch;

    void observeInternalBlock()
    {
    	Chunk chunk = static_cast<Child*>(this)->getChunk();

        totalLength += chunk.length;

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

    void observeFirstBlock()
    {
    	uint8_t firstByte = *static_cast<Child*>(this)->data;

    	NET_ASSERT((firstByte & 0xf0) == 0x40); // IPv4 only for now.

        remainingLength = (firstByte & 0xf) << 2;
        totalLength = 0;
        offsetOdd = false;
        observeInternalBlock();
    }

    bool finish() {
    	return static_cast<Child*>(this)->skipAhead(static_cast<uint16_t>(remainingLength));
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

    	auto chunk = static_cast<Child*>(this)->getChunk();
        this->consume(chunk.start, chunk.length);

        remainingLength = newLength - chunk.length;
        offsetOdd = false;
    }
};

template<class S, class... Args>
class Network<S, Args...>::ValidatorPacketStream: public PacketStreamBase<ChecksumValidatorObserver> {
public:
	inline ValidatorPacketStream() = default;
	inline ValidatorPacketStream(const Packet& p) {
		this->init(p);
	}

	inline ValidatorPacketStream(const Packet& p, uint16_t offset) {
		this->init(p, offset);
	}

};

#endif /* VALIDATORPACKETSTREAM_H_ */
