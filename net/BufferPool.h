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

template<class Os, size_t nBlocks, size_t blockDataSize, class AlignmentType = void*>
class BufferPool: public Os::template IoChannelBase<BufferPool<Os, nBlocks, blockDataSize, AlignmentType>>, Os::Event
{
	friend class BufferPool::IoChannelBase;

public:
	struct Block {
		 char data[blockDataSize] alignas(AlignmentType);
		 Block* next;
	};

	class Data: public Os::IoJob::Data {
		friend pet::LinkedList<Data>;
		Data* next;
	public:
		union {
			size_t size;
			Block* first;
		};
	};

private:
	template<class Value> using Atomic = typename Os::template Atomic<Value>;

	Atomic<size_t> nFree;
	Atomic<Block*> head;
	Block blocks[nBlocks];
	pet::LinkedList<Data> items;

	inline Block* take() {
		Block* ret;

		head([&ret](Block* old, Block* &result){
			result = old->next;
			ret = old;
			return true;
		});

		return ret;
	}

	inline void put(Block* b, Block** nextOfLast) {
		head([b, nextOfLast](Block* old, Block* &result){
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

	inline bool tryToSatisfy(Data* data) {
		if(Block* first = allocate(data->size)) {
			data->first = first;
			this->jobDone(data);
			return true;
		}

		return false;
	}

	static void blocksReclaimed(typename Os::Event* event, uintptr_t arg) {
		BufferPool* self = static_cast<BufferPool*>(event);

		while(Data* job = self->items.iterator().current()) {
			if(!self->tryToSatisfy(static_cast<Data*>(job)))
				break;
		}
	}

	inline bool hasJob() {
		return this->items.iterator().current() != nullptr;
	}

	inline bool addItem(typename Os::IoJob::Data* job)
	{
		Data* data = static_cast<Data*>(job);

		if(!items.iterator().current()) {
			if(tryToSatisfy(data))
				return true;
		}

		return items.addBack(data);
	}

	inline bool removeItem(typename Os::IoJob::Data* job) {
		Data* data = static_cast<Data*>(job);

		bool isFirst = data == items.iterator().current();
		bool ok = items.remove(data);

		if(ok && isFirst) {
			while(Data* item = items.iterator().current()) {
				if(!tryToSatisfy(item))
					break;
			}
		}

		return ok;
	}

	inline void enableProcess() {}
	inline void disableProcess() {}

public:
	inline void init() {
		nFree = nBlocks;
		head = blocks;

		for(size_t i = 0; i < nBlocks - 1; i++)
			blocks[i].next = &blocks[i+1];

		blocks[nBlocks - 1].next = nullptr;

		BufferPool::IoChannelBase::init();
	}

	inline void reclaim(Block *first) {
		size_t n = 0;
		Block** nextOfLast;

		for(Block* b = first;; b = b->next) {
			n++;
			if(!b->next) {
				nextOfLast = &b->next;
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
