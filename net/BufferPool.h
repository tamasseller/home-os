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

#include "DataContainer.h"

template<class Os, size_t nBlocks, class Data>
class BufferPool: public Os::template IoChannelBase<BufferPool<Os, nBlocks, Data>>, Os::Event
{
	friend class BufferPool::IoChannelBase;

	union Block {
		DataContainer<Data> data;
		union Block* next;
	};
public:

	class Allocator {
		friend class BufferPool;

		Block* first;
	public:
		template<class... C>
		inline Data* get(C... c) {
			if(Block* ret = first) {
				first = first->next;
				ret->data.init(c...);
				return ret->data.getData();
			}

			return nullptr;
		}

		inline bool hasMore() {
			return first != nullptr;
		}

		inline void freeSpare(BufferPool* pool)
		{
			if(Block *last = first) {
				size_t n = 1;
				while(last->next) {
					last = last->next;
					n++;
				}

				pool->reclaim(first, last, n);
			}
		}
	};

	class Deallocator {
		Block *first, *last;
		size_t n;
	public:
		inline Deallocator(Data *data) {
			Block* block = reinterpret_cast<Block*>(data);
			block->data.destroy();
			last = this->first = block;
			n = 1;
		}

		inline void take(Data* data) {
			Block* block = reinterpret_cast<Block*>(data);
			block->data.destroy();
			last->next = block;
			last = block;
			n++;
		}

		inline void deallocate(BufferPool* pool) {
			pool->reclaim(first, last, n);
		}
	};

	/**
	 * Allocation request parameters.
	 */
	class IoData: public Os::IoJob::Data {
		template<class, size_t, size_t, class Header, Header* &(*Next)(Header*)> class BufferPool;
		friend pet::LinkedList<IoData>;
		IoData* next;
	public:
		union {
			size_t size;
			Allocator allocator;
		};
	};

	/**
	 * Definition of the storage area.
	 *
	 * Can be used to indirectly supply the storage area to be operated on,
	 * by the application to specify special attributes on the declaration.
	 */
	typedef Block Storage[nBlocks];

private:
	template<class Value> using Atomic = typename Os::template Atomic<Value>;

	/**
	 * Number of free blocks.
	 */
	Atomic<size_t> nFree;

	/**
	 * The first one in the chain of free blocks.
	 */
	Atomic<Block*> head;
	pet::LinkedList<IoData> items;

	inline Block* take() {
		Block* ret;

		head([&ret](Block* old, Block* &result){
			result = old->next;
			ret = old;
			return true;
		});

		return ret;
	}

	inline void put(Block* first, Block* last) {
		head([first, last](Block* old, Block* &result){
			last->next = old;
			result = first;
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

	inline Block* allocate(size_t n)
	{
		Block* ret = nullptr;

		if(reserve(n)){
			Block* last = ret = take();

			while(--n) {
				Block* next = take();
				last->next = next;
				last = next;
			}

			last->next = nullptr;
		}

		return ret;
	}

	inline bool tryToSatisfy(IoData* data) {
		if(Block* first = allocate(data->size)) {
			data->allocator.first = first;
			this->jobDone(data);
			return true;
		}

		return false;
	}

	inline void reclaim(Block *first, Block *last, size_t n) {
		this->put(first, last);
		this->unReserve(n);

		if(this->hasJob())
			Os::submitEvent(this);
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
	inline size_t statUsed() {
		return nBlocks - nFree;
	}

	inline void init(Storage &blocks) {
		nFree = nBlocks;
		head = blocks;

		for(size_t i = 0; i < nBlocks - 1; i++)
			blocks[i].next = &blocks[i+1];

		blocks[nBlocks - 1].next = nullptr;

		BufferPool::IoChannelBase::init();
	}

	inline Allocator allocateDirect(size_t size)
	{
		Allocator ret;

		if(items.iterator().current())
			ret.first = nullptr;
		else
			ret.first = allocate(size);

		return ret;
	}

	inline BufferPool(): Os::Event(&BufferPool::blocksReclaimed) {}
};

#endif /* BUFFERPOOL_H_ */
