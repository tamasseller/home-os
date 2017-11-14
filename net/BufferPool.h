/*
 * BufferPool.h
 *
 *  Created on: 2017.11.13.
 *      Author: tooma
 */

#ifndef BUFFERPOOL_H_
#define BUFFERPOOL_H_

template<class Os, size_t nBlocks, size_t blockDataSize>
struct BufferPool: Os::IoChannel
{
	struct Block {
		 Block* next;
		 char data[blockDataSize];
	};

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

public:
	struct Chain {
		Block *first = nullptr, *last = nullptr;

		inline size_t length()
		{
			size_t ret = 0;

			for(Block* i = first; i; i = i->next)
				ret++;

			return ret;
		}
	};

	inline BufferPool(): nFree(nBlocks), head(blocks)
	{
		for(size_t i = 0; i < nBlocks - 1; i++)
			blocks[i].next = &blocks[i+1];

		blocks[nBlocks - 1].next = nullptr;
	}

	inline Chain allocate(size_t n)
	{
		Chain ret;

		if(reserve(n)){
			ret.first = ret.last = take();

			while(--n) {
				Block* next = take();
				ret.last->next = next;
				ret.last = next;
			}

			ret.last->next = nullptr;
		}

		return ret;
	}

	inline void reclaim(const Chain &chain) {
		size_t n = 0;
		Block** nextOfLast;

		for(Block* b = chain.first;; b = b->next) {
			n++;
			if(!b->next) {
				nextOfLast = &b->next;
				break;
			}
		}

		put(chain.first, nextOfLast);
		unReserve(n);
	};
};

#endif /* BUFFERPOOL_H_ */
