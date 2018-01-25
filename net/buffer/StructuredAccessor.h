/*
 * StructuredAccessor.h
 *
 *  Created on: 2018.01.24.
 *      Author: tooma
 */

#ifndef STRUCTUREDACCESSOR_H_
#define STRUCTUREDACCESSOR_H_


template<class... Fields> struct StructuredReader {};

template<class F, class... Rest> struct StructuredReader<F, Rest...> {
	template<size_t at, class S, class Ret>
	static bool read(S& s, Ret& ret)
	{
		if(at < F::offset)
			if(!s.skipAhead(F::offset - at))
				return false;

		if(!ret.F::read(s))
			return false;

		return StructuredReader<Rest...>::template read<F::offset + F::length>(s, ret);
	}
};

template<> struct StructuredReader<> {
	template<size_t, class S, class Ret>
	static inline bool read(S& s, Ret& ret) {
		return true;
	}
};

template<class... Fields> struct StructuredAccessor: Fields... {
	template<class F> decltype(F::data) get() {
		return static_cast<F*>(this)->data;
	}

	template<class Stream>
	bool extract(Stream& s) {
		return StructuredReader<Fields...>::template read<0>(s, *this);
	}
};

template<size_t off>
struct Field8 {
	uint8_t data;
	static constexpr auto offset = off;
	static constexpr auto length = 1;

	template<class Stream>
	inline bool read(Stream& s) {
		return s.read8(data);
	}
};

template<size_t off>
struct Field16 {
	uint16_t data;
	static constexpr auto offset = off;
	static constexpr auto length = 2;

	template<class Stream>
	inline bool read(Stream& s) {
		return s.read16net(data);
	}
};

template<size_t off>
struct Field32 {
	uint32_t data;
	static constexpr auto offset = off;
	static constexpr auto length = 4;

	template<class Stream>
	inline bool read(Stream& s) {
		return s.read32net(data);
	}
};

template<size_t off>
struct EndMarker {
    static constexpr auto offset = off;
    static constexpr auto length = 0;

    template<class Stream>
    inline bool read(Stream& s) {
        return true;
    }
};

#endif /* STRUCTUREDACCESSOR_H_ */
