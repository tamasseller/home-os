/*
 * SeqNum.h
 *
 *  Created on: 2018.02.22.
 *      Author: tooma
 */

#ifndef SEQNUM_H_
#define SEQNUM_H_

struct SeqNum {
	uint32_t value;

	explicit SeqNum(uint32_t x): value(x) {}
	SeqNum() = default;

	operator uint32_t() const {
		return value;
	}

	bool operator <(const SeqNum &o) const {
		return int32_t(value - o.value) < 0;
	}

	bool operator >(const SeqNum &o) const {
		return int32_t(value - o.value) > 0;
	}

	bool operator <=(const SeqNum &o) const {
		return int32_t(value - o.value) <= 0;
	}

	bool operator >=(const SeqNum &o) const {
		return int32_t(value - o.value) >= 0;
	}

	SeqNum operator +(const SeqNum &o) const {
		return SeqNum(value + o.value);
	}

	SeqNum operator -(const SeqNum &o) const {
		return SeqNum(value - o.value);
	}

	void operator += (const SeqNum &o) {
		value += o.value;
	}

	void operator -= (const SeqNum &o) {
		value -= o.value;
	}

	SeqNum operator = (uint32_t x)
	{
		value = x;
		return *this;
	}
};



#endif /* SEQNUM_H_ */
