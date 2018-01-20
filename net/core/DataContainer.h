/*
 * DataContainer.h
 *
 *  Created on: 2017.12.02.
 *      Author: tooma
 */

#ifndef DATACONTAINER_H_
#define DATACONTAINER_H_

template<class Data>
class DataContainer {
	struct Hax: Data {
		static void* operator new(size_t, void* r) {
			return r;
		}
	};

	char data[sizeof(Data)] alignas(Data);

public:
	template<class... C>
	void init(C... c) {
		new(data) Hax(c...);
	}

	void destroy() {
		reinterpret_cast<Hax*>(data)->~Hax();
	}

	Data* getData() {
		return reinterpret_cast<Hax*>(data);
	}
};

#endif /* DATACONTAINER_H_ */
