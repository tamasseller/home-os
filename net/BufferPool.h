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

template<class Os, size_t nBlocks, class Data, size_t txQuota = nBlocks, size_t rxQuota = nBlocks>
class BufferPool: public Os::template SynchronousIoChannelBase<BufferPool<Os, nBlocks, Data, txQuota, rxQuota>>, Os::Event
{
	friend class BufferPool::SynchronousIoChannelBase;

	union Block {
		DataContainer<Data> data;
		union Block* next;
	};

public:
	enum class Quota: uint16_t { Rx, Tx, None };

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

		template<Quota quota>
		inline void freeSpare(BufferPool* pool)
		{
			if(Block *last = first) {
				size_t n = 1;
				while(last->next) {
					last = last->next;
					n++;
				}

				pool->reclaim<quota>(first, last, n);
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

		template<Quota quota>
		inline void deallocate(BufferPool* pool) {
			pool->reclaim<quota>(first, last, n);
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
			struct {
				uint16_t size;
				Quota quota;
			} request;

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
	struct ReservationQueue;
	struct AtomicPool;

	AtomicPool pool;			///< The Pool of buffer blocks,
	ReservationQueue memQueue;	///< Requests waiting for free buffer blocks.
	ReservationQueue txQueue;	///< Requests waiting due to TX quota exceeded.
	ReservationQueue rxQueue;	///< Requests waiting due to RX quota exceeded.

	constexpr inline ReservationQueue* getQotaQueue(Quota quota) {
		return (quota == Quota::Tx) ?
				&txQueue :
				((quota == Quota::Rx) ?
						&rxQueue :
						nullptr);
	}

	inline void updateQuota(ReservationQueue* queue) {
		while(IoData* data = queue->fetchCompleted()) {
			if(memQueue.considerRequest(data)) {
				data->allocator.first = pool.allocate(data->request.size);
				this->jobDone(data);
			}
		}
	}

	inline void updateMem() {
		while(IoData* data = memQueue.fetchCompleted()) {
			data->allocator.first = pool.allocate(data->request.size);
			this->jobDone(data);
		}
	}

	template<Quota quota>
	inline void reclaim(Block *first, Block *last, size_t n) {
		pool.put(first, last);
		memQueue.release(n);

		if(ReservationQueue* quotaQueue = getQotaQueue(quota))
			quotaQueue->release(n);

		if(	this->memQueue.hasJob() ||
			this->txQueue.hasJob() ||
			this->rxQueue.hasJob())
			Os::submitEvent(this);
	}

	static void blocksReclaimed(typename Os::Event* event, uintptr_t arg) {
		BufferPool* self = static_cast<BufferPool*>(event);
		self->updateMem();
		self->updateQuota(&self->txQueue);
		self->updateQuota(&self->rxQueue);
	}

	inline bool addItem(typename Os::IoJob::Data* job)
	{
		IoData* data = static_cast<IoData*>(job);

		if(ReservationQueue *queue = getQotaQueue(data->request.quota)) {
			if(!queue->considerRequest(data))
				return true;
		}

		if(memQueue.considerRequest(data)) {
			data->allocator.first = pool.allocate(data->request.size);
			this->jobDone(data);
		}

		return true;
	}

	inline bool removeCanceled(typename Os::IoJob::Data* job) {
		IoData* data = static_cast<IoData*>(job);

		auto memRet = memQueue.remove(data);
		if(memRet == ReservationQueue::RemoveResult::First) {
			updateMem();
		} else if(memRet == ReservationQueue::RemoveResult::NotFound) {
			auto txRet = txQueue.remove(data);
			if(txRet == ReservationQueue::RemoveResult::First) {
				updateQuota(&txQueue);
			} else if(txRet == ReservationQueue::RemoveResult::NotFound) {
				if(rxQueue.remove(data) == ReservationQueue::RemoveResult::First)
					updateQuota(&rxQueue);
			}
		}

		return true;
	}

public:
	inline size_t statUsed() {
		return nBlocks - memQueue.stat();
	}

	inline size_t statTxUsed() {
		return txQuota - txQueue.stat();
	}

	inline size_t statRxUsed() {
		return rxQuota - rxQueue.stat();
	}

	inline void init(Storage &blocks) {
		memQueue.init(nBlocks);
		txQueue.init(txQuota);
		rxQueue.init(rxQuota);
		pool.init(blocks);

		for(size_t i = 0; i < nBlocks - 1; i++)
			blocks[i].next = &blocks[i+1];

		blocks[nBlocks - 1].next = nullptr;

		BufferPool::SynchronousIoChannelBase::init();
	}

	template<Quota quota>
	inline Allocator allocateDirect(size_t size)
	{
		Allocator ret;
		ret.first = nullptr;

		if(ReservationQueue* queue = getQotaQueue(quota)) {
			if(queue->passthrough(size)) {
				if(memQueue.passthrough(size))
					ret.first = pool.allocate(size);
				else
					queue->release(size);
			}
				return ret;
		} else {
			if(memQueue.passthrough(size))
				ret.first = pool.allocate(size);
		}


		return ret;
	}

	template<Quota quota>
	inline bool allocateDirectOrDeferred(
			typename Os::IoJob::Launcher *launcher,
			typename Os::IoJob::Callback callback,
			IoData* data,
			uint16_t size)
	{
		auto ret = allocateDirect<quota>(size);

		if(ret.hasMore()) {
			data->allocator = ret;
			return callback(launcher, launcher->getJob(), Os::IoJob::Result::Done);
		} else {
			data->request.size = size;
			data->request.quota = quota;
            launcher->launch(this, callback, data);
            return true;
		}
	}

	inline BufferPool(): Os::Event(&BufferPool::blocksReclaimed) {}
};

template<class Os, size_t nBlocks, class Data, size_t txQuota, size_t rxQuota>
class BufferPool<Os, nBlocks, Data, txQuota, rxQuota>::AtomicPool
{
	Atomic<Block*> head;

	inline Block* take() {
		Block* ret;

		head([&ret](Block* old, Block* &result){
			result = old->next;
			ret = old;
			return true;
		});

		return ret;
	}

public:
	inline Block* allocate(size_t n)
	{
		Block *ret, *last;
		last = ret = take();

		while(--n) {
			Block* next = take();
			last->next = next;
			last = next;
		}

		last->next = nullptr;

		return ret;
	}

	inline void put(Block* first, Block* last) {
		head([first, last](Block* old, Block* &result){
			last->next = old;
			result = first;
			return true;
		});
	}

	inline void init(Storage &blocks) {
		head = blocks;
	}
};

template<class Os, size_t nBlocks, class Data, size_t txQuota, size_t rxQuota>
class BufferPool<Os, nBlocks, Data, txQuota, rxQuota>::ReservationQueue
{
	Atomic<size_t> nFree;			///< Number of free blocks.
	pet::LinkedList<IoData> items;	///< Requests waiting for free buffers.

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

public:
	inline void init(size_t n) {
		nFree = n;
	}

	inline size_t stat() {
		return nFree;
	}

	inline bool passthrough(size_t n) {
		if(items.iterator().current())
			return false;

		return reserve(n);
	}

	inline bool considerRequest(IoData* req) {
		if(passthrough(req->request.size))
			return true;

		items.addBack(req);
		return false;
	}

	inline IoData* fetchCompleted()
	{
		auto it = items.iterator();

		if(IoData* data = it.current()) {
			if(reserve(data->request.size)) {
				it.remove();
				return data;
			}
		}

		return nullptr;
	}

	inline void release(size_t n) {
		nFree([n](size_t old, size_t &result){
			result = old + n;
			return true;
		});
	}

	enum class RemoveResult {
		NotFound, First, NonFirst
	};

	inline RemoveResult remove(IoData* data) {
		auto it = items.iterator();

		if(it.current() == data) {
			it.remove();
			return RemoveResult::First;
		}

		for(;it.current();it.step()) {
			if(it.current() == data) {
				it.remove();
				return RemoveResult::NonFirst;
			}
		}

		return RemoveResult::NotFound;
	}

	bool hasJob() {
		return items.iterator().current();
	}
};


#endif /* BUFFERPOOL_H_ */
