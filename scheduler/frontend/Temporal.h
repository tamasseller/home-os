/*
 * Temporal.h
 *
 *  Created on: 2018.03.07.
 *      Author: tooma
 */

#ifndef TEMPORAL_H_
#define TEMPORAL_H_

#include <stdint.h>

namespace home {

template<uint32_t clockHz>
class Duration {
	template<uint32_t> friend class Duration;
	template<class> friend class Time;

	template <uint32_t foreignHz>
	constexpr static inline uint32_t convertFrom(uint32_t value) {
		return uint32_t((uint64_t(value) * clockHz + foreignHz - 1) / foreignHz);
	}

protected:
	uint32_t count;

public:
	inline Duration() = default;
	constexpr inline explicit Duration(uint32_t count): count(count) {};

	template<uint32_t otherHz>
	constexpr inline Duration(const Duration<otherHz> &o): count(convertFrom<otherHz>(o.count)) {}

	inline operator uint32_t() const {
		return count;
	}

	inline Duration operator +(const Duration &o) const {
		return Duration(count + o.count);
	}

	inline Duration operator -(const Duration &o) const {
		return Duration(count - o.count);
	}

	inline Duration operator *(uint32_t s) const {
		return Duration(count * s);
	}

	inline Duration operator /(uint32_t s) const {
		return Duration(count / s);
	}

	inline bool operator ==(const Duration &o) const {
		return count == o.count;
	}

	inline bool operator !=(const Duration &o) const {
		return count != o.count;
	}

	inline bool operator <(const Duration &o) const {
		return count < o.count;
	}

	inline bool operator >(const Duration &o) const {
		return count > o.count;
	}

	inline bool operator <=(const Duration &o) const {
		return count <= o.count;
	}

	inline bool operator >=(const Duration &o) const {
		return count >= o.count;
	}

	template<uint32_t otherHz>
	inline bool operator ==(const Duration<otherHz> &o) const {
		if(otherHz > clockHz)
			return o == *this;

		return *this == Duration(o);
	}

	template<uint32_t otherHz>
	inline bool operator !=(const Duration<otherHz> &o) const {
		if(otherHz > clockHz)
			return o != *this;

		return *this != Duration(o);
	}

	template<uint32_t otherHz>
	inline bool operator <(const Duration<otherHz> &o) const {
		if(otherHz > clockHz)
			return o > *this;

		return *this < Duration(o);
	}

	template<uint32_t otherHz>
	inline bool operator >(const Duration<otherHz> &o) const {
		if(otherHz > clockHz)
			return o < *this;

		return *this > Duration(o);
	}

	template<uint32_t otherHz>
	inline bool operator <=(const Duration<otherHz> &o) const {
		if(otherHz > clockHz)
			return o >= *this;

		return *this <= Duration(o);
	}

	template<uint32_t otherHz>
	inline bool operator >=(const Duration<otherHz> &o) const {
		if(otherHz > clockHz)
			return o <= *this;

		return *this >= Duration(o);
	}

	inline Duration operator += (const Duration &o) {
		count += o.count;
		return *this;
	}

	inline Duration operator -= (const Duration &o) {
		count -= o.count;
		return *this;
	}

	inline Duration operator *=(uint32_t s) {
		count *= s;
		return *this;
	}

	inline Duration operator /=(uint32_t s) {
		count /= s;
		return *this;
	}
};

template<class Base>
class Time: Base {
	friend Base;
public:
	inline Time() = default;
	constexpr inline explicit Time(uint32_t count): Base(count) {};

	template<uint32_t otherHz>
	constexpr inline Time(const Duration<otherHz> &o): Base(o) {}

	using Base::operator uint32_t;

	inline bool operator ==(const Time &o) const {
		return this->Base::operator==(o);
	}

	inline bool operator !=(const Time &o) const {
		return this->Base::operator!=(o);
	}

	inline Time operator +(const Base &o) const {
		return Time(this->count + o.count);
	}

	inline Time operator -(const Base &o) const {
		return Time(this->count - o.count);
	}

	inline Time operator += (const Base &o) {
		*static_cast<Base*>(this) += o;
		return *this;
	}

	inline Time operator -= (const Base &o) {
		*static_cast<Base*>(this) -= o;
		return *this;
	}

	inline bool operator <(const Time &o) const {
		return int32_t(this->count - o.count) < 0;
	}

	inline bool operator >(const Time &o) const {
		return int32_t(this->count - o.count) > 0;
	}

	inline bool operator <=(const Time &o) const {
		return int32_t(this->count - o.count) <= 0;
	}

	inline bool operator >=(const Time &o) const {
		return int32_t(this->count - o.count) >= 0;
	}
};

using Seconds   = Duration<1u>;
using MilliSecs = Duration<1000u>;
using MicroSecs = Duration<1000u * 1000u>;
using NanoSecs  = Duration<1000u * 1000u * 1000u>;
}

#endif /* TEMPORAL_H_ */
