/*
 * StructuredAccessor.h
 *
 *  Created on: 2018.01.24.
 *      Author: tooma
 */

#ifndef STRUCTUREDACCESSOR_H_
#define STRUCTUREDACCESSOR_H_


template<template<class> class Work, class... Fields> struct StructuredWorker {};

template<template<class> class Work, class F, class... Rest> struct StructuredWorker<Work, F, Rest...> {
	template<size_t at, class S, class Ret>
	static bool work(S& s, Ret& ret)
	{
        static_assert(at <= F::offset, "Wrong field order");

		if(at < F::offset)
			if(!s.skipAhead(F::offset - at))
				return false;

		if(!Work<F>::work(ret, s))
			return false;

		return StructuredWorker<Work, Rest...>::template work<F::offset + F::length>(s, ret);
	}
};

template<template<class> class Work> struct StructuredWorker<Work> {
	template<size_t, class S, class Ret>
	static inline bool work(S& s, Ret& ret) {
		return true;
	}
};

template<class... Fields> struct GetLastField {};
template<class F, class... Rest> struct GetLastField<F, Rest...>: GetLastField<Rest...> {};
template<class F> struct GetLastField<F> {
	typedef F LastField;
};

template<class... Fields> struct StructuredAccessor: GetLastField<Fields...>, Fields... {
    template<class Field> struct Read {
        template<class Stream>
        static inline bool work(Field& f, Stream& s) {
            return f.read(s);
        }
    };

    template<class Field> struct Write {
        template<class Stream>
        static inline bool work(Field& f, Stream& s) {
            return f.write(s);
        }
    };

public:
	template<class F> decltype(F::data)& get() {
		return static_cast<F*>(this)->data;
	}

	template<class Stream>
	bool extract(Stream& s) {
		return StructuredWorker<Read, Fields...>::template work<0>(s, *this);
	}

    template<class Stream>
    bool fill(Stream& s) {
        return StructuredWorker<Write, Fields...>::template work<0>(s, *this);
    }

    template<class FirstField, class Stream>
    bool fillFrom(Stream& s) {
        return StructuredWorker<Write, Fields...>::template work<FirstField::offset + FirstField::length>(s, *this);
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

    template<class Stream>
    inline bool write(Stream& s) {
        return s.write8(data);
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

    template<class Stream>
    inline bool write(Stream& s) {
        return s.write16net(data);
    }
};

template<size_t off>
struct Field16raw {
    uint16_t data;
    static constexpr auto offset = off;
    static constexpr auto length = 2;

    template<class Stream>
    inline bool read(Stream& s) {
        return s.read16raw(data);
    }

    template<class Stream>
    inline bool write(Stream& s) {
        return s.write16raw(data);
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

    template<class Stream>
    inline bool write(Stream& s) {
        return s.write32net(data);
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

    template<class Stream>
    inline bool write(Stream& s) {
        return true;
    }
};

#endif /* STRUCTUREDACCESSOR_H_ */
