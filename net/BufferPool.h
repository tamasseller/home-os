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
class BufferPool: public Os::IoChannel, Os::Event
{
public:
	struct Block {
		 char data[blockDataSize] alignas(AlignmentType);
		 Block* next;
	};

private:
	template<class Value> using Atomic = typename Os::template Atomic<Value>;

	Atomic<size_t> nFree;
	Atomic<Block*> head;
	Block blocks[nBlocks];

	Block* take() {
		Block* ret;

		head([&ret](Block* old, Block* &result){
			result = old->next;
			ret = old;
			return true;
		});

		return ret;
	}

	void put(Block* b, Block** nextOfLast) {
		head([b, nextOfLast](Block* old, Block* &result){
			*nextOfLast = old;
			result = b;
			return true;
		});
	}

	bool reserve(size_t n) {
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

	void unReserve(size_t n) {
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

	bool tryToSatisfy(typename Os::IoJob* job ) {
		size_t param = reinterpret_cast<uintptr_t>(job->param);

		if(Block* first = allocate(param)) {
			job->param = reinterpret_cast<uintptr_t>(first);
			this->jobDone(job);
			return true;
		}

		return false;
	}

	static void blocksReclaimed(typename Os::Event* event, uintptr_t arg) {
		BufferPool* self = static_cast<BufferPool*>(event);

		while(typename Os::IoJob* job = self->jobs.front()) {
			if(!self->tryToSatisfy(job))
				break;
		}
	}

	virtual bool addJob(typename Os::IoJob* job)
	{
		if(!this->jobs.front()) {
			if(tryToSatisfy(job))
				return true;
		}

		return this->jobs.addBack(job);
	}

	virtual bool removeJob(typename Os::IoJob* job) {
		bool isFirst = job == this->jobs.front();
		bool ok = this->jobs.remove(job);

		if(ok && isFirst) {
			while(typename Os::IoJob* job = this->jobs.front()) {
				if(!tryToSatisfy(job))
					break;
			}
		}

		return ok;
	}

	virtual void enableProcess() {}
	virtual void disableProcess() {}

public:
	inline void init() {
		nFree = nBlocks;
		head = blocks;

		for(size_t i = 0; i < nBlocks - 1; i++)
			blocks[i].next = &blocks[i+1];

		blocks[nBlocks - 1].next = nullptr;

		Os::IoChannel::init();
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
