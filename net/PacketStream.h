/*
 * PacketStream.h
 *
 *  Created on: 2017.12.03.
 *      Author: tooma
 */

#ifndef PACKETSTREAM_H_
#define PACKETSTREAM_H_

template<class S, class... Args>
class Network<S, Args...>::PacketStream: PacketDisassembler
{
	char *data, *limit;

	constexpr inline uint16_t spaceLeft() const {
		return static_cast<uint16_t>(limit - data);
	}

	inline bool takeNext() {
		if(!PacketDisassembler::advance())
			return false;

		Chunk ret = this->getCurrentChunk();
		data = ret.start;
		limit = ret.end;
		return true;
	}

public:
	inline void init(const Packet& p){
		PacketDisassembler::init(p);
		/*
		 * Any packet must have headers that can only be written to in-line blocks,
		 * so it is safe to assume that the first block contains in-line data.
		 */
		data = this->current->getData();
		limit = data + this->current->getSize();
	}

	inline Chunk getChunk()
	{
		if(!spaceLeft()) {
			if(!takeNext())
				return Chunk {nullptr, nullptr};
		}

		return Chunk {data, limit};
	}

	inline void advance(uint16_t x) {
		data += x;
	}

	inline uint16_t copyOut(char* output, uint16_t outputLength) {
		uint16_t done = 0;
		while(const uint16_t leftoverLength = static_cast<uint16_t>(outputLength - done)) {
			if(const auto space = spaceLeft()) {
				const uint16_t runLength = (space < leftoverLength) ? space : leftoverLength;
				memcpy(output, data, runLength);
				done = static_cast<uint16_t>(done + runLength);
				output += runLength;
				data += runLength;
			} else {
				if(!takeNext())
					break;
			}
		}

		return done;
	}

	inline bool atEop() {
		return !spaceLeft() && this->current->isEndOfPacket();
	}

 	inline uint8_t read8() {
		uint8_t ret;

		if(spaceLeft() > 0)
			ret = *data++;
		else
			copyOut(reinterpret_cast<char*>(&ret), 1);

		return ret;
	}

	inline uint16_t read16() {
		uint16_t ret;

		if(spaceLeft() > 1) {
			uint16_t b1 = static_cast<uint8_t>(*data++);
			uint16_t b2 = static_cast<uint8_t>(*data++);
			ret = static_cast<uint16_t>(b1 << 8 | b2);
		} else {
			copyOut(reinterpret_cast<char*>(&ret), 2);
			ret = correctEndian(ret);
		}

		return ret;
	}

	inline uint32_t read32() {
		uint32_t ret;

		if(spaceLeft() > 3) {
			uint32_t b1 = static_cast<uint8_t>(*data++);
			uint32_t b2 = static_cast<uint8_t>(*data++);
			uint32_t b3 = static_cast<uint8_t>(*data++);
			uint32_t b4 = static_cast<uint8_t>(*data++);
			ret = b1 << 24 | b2 << 16 | b3 << 8 | b4;
		} else {
			copyOut(reinterpret_cast<char*>(&ret), 4);
			ret = correctEndian(ret);
		}

		return ret;
	}
};

#endif /* PACKETSTREAM_H_ */
