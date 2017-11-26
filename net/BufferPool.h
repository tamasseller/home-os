/*******************************************************************************
 *
 * Copyright (c) 2017 Seller Tamás. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *******************************************************************************/

#ifndef BUFFERPOOL_H_
#define BUFFERPOOL_H_

template<class Header>
static inline Header* &trivialNextAccessor(Header* header) {
	return header->next;
}

struct TrivialHeader {
	TrivialHeader* next;
};

template<class Os>
class BufferPoolIoData: public Os::IoJob::Data {
	template<class, size_t, size_t, class Header, Header* &(*Next)(Header*)> class BufferPool;
	friend pet::LinkedList<BufferPoolIoData>;
	BufferPoolIoData* next;
public:
	union {
		size_t size;
		void* first;
	};
};

template<class Os, size_t nBlocks, size_t blockDataSize, class Header = TrivialHeader, Header* &(*accessNext)(Header* header) = &trivialNextAccessor<Header>>
class BufferPool: public Os::template IoChannelBase<BufferPool<Os, nBlocks, blockDataSize, Header, accessNext>>, Os::Event
{
	friend class BufferPool::IoChannelBase;

public:
	using IoData = BufferPoolIoData<Os>;

	struct Block: Header {
		 char data[blockDataSize] alignas(Header);

		 Header* &getNext() {
			 return accessNext(this);
		 }
	};

	typedef Block Storage[nBlocks];

private:
	template<class Value> using Atomic = typename Os::template Atomic<Value>;

	Atomic<size_t> nFree;
	Atomic<Header*> head;
	pet::LinkedList<IoData> items;

	inline Header* take() {
		Header* ret;

		head([&ret](Header* old, Header* &result){
			result = accessNext(old);
			ret = old;
			return true;
		});

		return ret;
	}

	inline void put(Header* b, Header** nextOfLast) {
		head([b, nextOfLast](Header* old, Header* &result){
			*nextOfLast = old;
			result = b;
			return true;
		});
	}

	inline bool reserve(size_t n) {
		bool ok;

		nFree([&ok, n](size_t old, size_t &result){
			if(old >= n) {
				result = old - n;
				return ok = true;
			}
			return ok = false;
		});

		return ok;
	}

	inline void unReserve(size_t n) {
		nFree([n](size_t old, size_t &result){
			result = old + n;
			return true;
		});
	}

	inline Header* allocate(size_t n)
	{
		Header* ret = nullptr;

		if(reserve(n)){
			Header* last = ret = take();

			while(--n) {
				Header* next = take();
				accessNext(last) = next;
				last = next;
			}

			accessNext(last) = nullptr;
		}

		return ret;
	}

	inline bool tryToSatisfy(IoData* data) {
		const size_t nBlocksToAlloc = (data->size + blockDataSize - 1) / blockDataSize;

		if(Header* first = allocate(nBlocksToAlloc)) {
			data->first = static_cast<Block*>(first);
			this->jobDone(data);
			return true;
		}

		return false;
	}

	static void blocksReclaimed(typename Os::Event* event, uintptr_t arg) {
		BufferPool* self = static_cast<BufferPool*>(event);

		while(IoData* job = self->items.iterator().current()) {
			if(!self->tryToSatisfy(static_cast<IoData*>(job)))
				break;
		}
	}

	inline bool hasJob() {
		return this->items.iterator().current() != nullptr;
	}

	inline bool addItem(typename Os::IoJob::Data* job)
	{
		IoData* data = static_cast<IoData*>(job);

		if(!items.iterator().current()) {
			if(tryToSatisfy(data))
				return true;
		}

		return items.addBack(data);
	}

	inline bool removeItem(typename Os::IoJob::Data* job) {
		items.remove(static_cast<IoData*>(job));
		return true;
	}

	inline bool removeCanceled(typename Os::IoJob::Data* job) {
		IoData* data = static_cast<IoData*>(job);

		auto it = items.iterator();

		if(it.current() == data) {
			it.remove();
			while(IoData* item = items.iterator().current()) {
				if(!tryToSatisfy(item))
					break;
			}
			return true;
		}

		return items.remove(data);
	}

	inline void enableProcess() {}
	inline void disableProcess() {}

public:
	inline void init(Storage &blocks) {
		nFree = nBlocks;
		head = blocks;

		for(size_t i = 0; i < nBlocks - 1; i++)
			blocks[i].next = &blocks[i+1];

		blocks[nBlocks - 1].next = nullptr;

		BufferPool::IoChannelBase::init();
	}

	inline void reclaim(Block *first) {
		size_t n = 0;
		Header** nextOfLast;

		for(Header* b = first;; b = accessNext(b)) {
			n++;
			if(!accessNext(b)) {
				nextOfLast = &accessNext(b);
				break;
			}
		}

		put(first, nextOfLast);
		unReserve(n);

		if(this->hasJob())
			Os::submitEvent(this);
	};

	inline BufferPool(): Os::Event(&BufferPool::blocksReclaimed) {}
};



#endif /* BUFFERPOOL_H_ */
