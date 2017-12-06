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

#include "common/CommonTestUtils.h"

#include "BufferPool.h"

using Os = OsRr;

TEST_GROUP(NetBufferPool) {
	struct Dummy {
		Dummy* next;
	};

	typedef BufferPool<Os, 10, Dummy> Pool;

	class PoolJob: public Os::IoJob, Pool::IoData {
		static bool writeResult(typename Os::IoJob* item, typename Os::IoJob::Result result, typename Os::IoJob::Hook) {
			if(result == Os::IoJob::Result::Done) {
				auto self = static_cast<PoolJob*>(item);
				Dummy* current = self->result = self->allocator.get();

				while(Dummy* next = self->allocator.get()) {
					current->next = next;
					current = next;
				}

				current->next = nullptr;
			}

			return false;
		}

	public:
		inline bool start(Pool &pool, size_t n, Pool::Quota quota = Pool::Quota::Tx, Os::IoJob::Hook hook = nullptr) {
			result = nullptr;
			this->request.quota = quota;
			this->request.size = static_cast<uint16_t>(n);
			return submit(hook, &pool, &PoolJob::writeResult, this);
		}

		Dummy* volatile result;
		inline PoolJob(): result(nullptr) {}

		template<Pool::Quota quota = Pool::Quota::Tx>
		void destroy(Pool &pool) {
			Pool::Deallocator deallocator(result);

			for(Dummy* current = result->next; current;) {
				Dummy* next = current->next;
				deallocator.take(current);
				current = next;
			}

			deallocator.deallocate<quota>(&pool);
		}
	};
};

TEST(NetBufferPool, Simple) {
	struct Task: public TestTask<Task> {
		Pool pool;
		Pool::Storage storage;

		bool run() {
			pool.init(storage);

			PoolJob jobs[3];
			jobs[0].start(pool, 3);

			Os::sleep(1);
			if(jobs[0].result == nullptr) return bad;

			jobs[1].start(pool, 4);

			Os::sleep(1);
			if(jobs[1].result == nullptr) return bad;

			jobs[2].start(pool, 5);

			Os::sleep(1);
			if(jobs[2].result != nullptr) return bad;

			jobs[0].destroy(pool);

			Os::sleep(1);
			if(jobs[2].result == nullptr) return bad;

			return ok;
		}
	} task;

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(NetBufferPool, Direct) {
	struct Task: public TestTask<Task> {
		Pool pool;
		Pool::Storage storage;

		bool run() {
			pool.init(storage, 6, 8);

			auto r1 = pool.allocateDirect<Pool::Quota::Tx>(10);
			if(r1.hasMore()) return bad;

			auto r2 = pool.allocateDirect<Pool::Quota::Tx>(8);
			if(r2.hasMore()) return bad;

			auto r3 = pool.allocateDirect<Pool::Quota::Tx>(6);
			if(!r3.hasMore()) return bad;

			auto r4 = pool.allocateDirect<Pool::Quota::Rx>(8);
			if(r4.hasMore()) return bad;

			r3.freeSpare<Pool::Quota::Tx>(&pool);

			auto r5 = pool.allocateDirect<Pool::Quota::Rx>(8);
			if(!r5.hasMore()) return bad;

			auto r6 = pool.allocateDirect<Pool::Quota::None>(2);
			if(!r6.hasMore()) return bad;

			return ok;
		}
	} task;

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(NetBufferPool, Ordering) {
	struct Task: public TestTask<Task> {
		Pool pool;
		Pool::Storage storage;
		PoolJob jobs[4];

		bool run() {
			pool.init(storage);

			jobs[0].start(pool, 5);
			Os::sleep(1);
			if(jobs[0].result == nullptr) return bad;

			jobs[1].start(pool, 5);
			Os::sleep(1);
			if(jobs[1].result == nullptr) return bad;

			jobs[2].start(pool, 6);
			Os::sleep(1);
			if(jobs[2].result != nullptr) return bad;

			jobs[3].start(pool, 4);
			Os::sleep(1);
			if(jobs[3].result != nullptr) return bad;

			jobs[0].destroy(pool);
			Os::sleep(1);
			if(jobs[2].result != nullptr) return bad;
			if(jobs[3].result != nullptr) return bad;

			jobs[1].destroy(pool);
			Os::sleep(1);
			if(jobs[2].result == nullptr) return bad;
			if(jobs[3].result == nullptr) return bad;

			return ok;
		}
	} task;

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(NetBufferPool, LastCanceled) {
	struct Task: public TestTask<Task> {
		Pool pool;
		Pool::Storage storage;
		PoolJob jobs[4];

		bool run() {
			pool.init(storage);

			jobs[0].start(pool, 5);
			Os::sleep(1);
			if(jobs[0].result == nullptr) return bad;

			jobs[1].start(pool, 5);
			Os::sleep(1);
			if(jobs[1].result == nullptr) return bad;

			jobs[2].start(pool, 6);
			Os::sleep(1);
			if(jobs[2].result != nullptr) return bad;

			jobs[3].start(pool, 4);
			Os::sleep(1);
			if(jobs[3].result != nullptr) return bad;

			jobs[0].destroy(pool);
			Os::sleep(1);
			if(jobs[2].result != nullptr) return bad;
			if(jobs[3].result != nullptr) return bad;

			jobs[2].cancel();
			Os::sleep(1);
			if(jobs[3].result == nullptr) return bad;

			return ok;
		}
	} task;

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(NetBufferPool, FirstCanceled) {
	struct Task: public TestTask<Task> {
		Pool pool;
		Pool::Storage storage;
		PoolJob jobs[4];

		bool run() {
			pool.init(storage);

			jobs[0].start(pool, 5);
			Os::sleep(1);
			if(jobs[0].result == nullptr) return bad;

			jobs[1].start(pool, 5);
			Os::sleep(1);
			if(jobs[1].result == nullptr) return bad;

			jobs[2].start(pool, 6);
			Os::sleep(1);
			if(jobs[2].result != nullptr) return bad;

			jobs[3].start(pool, 4);
			Os::sleep(1);
			if(jobs[3].result != nullptr) return bad;

			jobs[0].destroy(pool);
			Os::sleep(1);
			if(jobs[2].result != nullptr) return bad;
			if(jobs[3].result != nullptr) return bad;

			jobs[3].cancel();
			jobs[1].destroy(pool);
			Os::sleep(1);
			if(jobs[2].result == nullptr) return bad;

			return ok;
		}
	} task;

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(NetBufferPool, Quota) {
	struct Task: public TestTask<Task> {
		Pool pool;
		Pool::Storage storage;

		bool run() {
			PoolJob jobs[4];
			pool.init(storage, 8, 8);					// M: 10, R: 8, T: 8

			jobs[0].start(pool, 6, Pool::Quota::Tx);	// M: 4, R: 8, T: 2
			Os::sleep(1);
			if(jobs[0].result == nullptr) return bad;

			jobs[1].start(pool, 4, Pool::Quota::Tx);	// M: 4, R: 8, T: 2 (Quota exceeded)
			Os::sleep(1);
			if(jobs[1].result != nullptr) return bad;

			jobs[2].start(pool, 7, Pool::Quota::Rx);	// M: 4, R: 1, T: 2
			Os::sleep(1);
			if(jobs[2].result != nullptr) return bad;

			jobs[3].start(pool, 6, Pool::Quota::Rx);	// M: 4, R: 1, T: 2
			Os::sleep(1);
			if(jobs[3].result != nullptr) return bad;

			jobs[0].destroy(pool);						// M: 4+6, R: 8, T: 2
			if(jobs[1].result != nullptr) return bad;	// Should move to buffers queue
			if(jobs[2].result == nullptr) return bad;
			if(jobs[3].result != nullptr) return bad;	// Should remain in quota queue

			jobs[2].destroy<Pool::Quota::Rx>(pool);
			if(jobs[1].result == nullptr) return bad;	// Should move to buffers queue
			if(jobs[3].result == nullptr) return bad;	// Should skip the buffer queue.

			return ok;
		}
	} task;

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(NetBufferPool, QuotaCancelRx) {
	struct Task: public TestTask<Task> {
		Pool pool;
		Pool::Storage storage;

		bool run() {
			PoolJob jobs[4];
			pool.init(storage, 8, 8);

			jobs[0].start(pool, 6, Pool::Quota::Rx);
			Os::sleep(1);
			if(jobs[0].result == nullptr) return bad;

			jobs[1].start(pool, 4, Pool::Quota::Rx);
			Os::sleep(1);
			if(jobs[1].result != nullptr) return bad;

			jobs[2].start(pool, 2, Pool::Quota::Rx);
			Os::sleep(1);
			if(jobs[2].result != nullptr) return bad;

			jobs[1].cancel();
			if(jobs[2].result == nullptr) return bad;

			return ok;
		}
	} task;

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(NetBufferPool, QuotaCancelMem) {
	struct Task: public TestTask<Task> {
		Pool pool;
		Pool::Storage storage;

		bool run() {
			PoolJob jobs[4];
			pool.init(storage, 8, 8);

			jobs[0].start(pool, 8, Pool::Quota::Tx);
			Os::sleep(1);
			if(jobs[0].result == nullptr) return bad;

			jobs[1].start(pool, 4, Pool::Quota::Rx);
			Os::sleep(1);
			if(jobs[1].result != nullptr) return bad;

			jobs[2].start(pool, 4, Pool::Quota::Rx);
			Os::sleep(1);
			if(jobs[2].result != nullptr) return bad;

			jobs[1].cancel();
			if(jobs[2].result != nullptr) return bad;

			jobs[0].destroy(pool);
			if(jobs[2].result == nullptr) return bad;

			return ok;
		}
	} task;

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(NetBufferPool, QuotaCancelMemNonFirst) {
	struct Task: public TestTask<Task> {
		Pool pool;
		Pool::Storage storage;

		bool run() {
			PoolJob jobs[4];
			pool.init(storage, 8, 8);

			jobs[0].start(pool, 8, Pool::Quota::Tx);
			Os::sleep(1);
			if(jobs[0].result == nullptr) return bad;

			jobs[1].start(pool, 4, Pool::Quota::Rx);
			Os::sleep(1);
			if(jobs[1].result != nullptr) return bad;

			jobs[2].start(pool, 4, Pool::Quota::Rx);
			Os::sleep(1);
			if(jobs[2].result != nullptr) return bad;

			jobs[2].cancel();
			if(jobs[1].result != nullptr) return bad;

			jobs[0].destroy(pool);
			if(jobs[1].result == nullptr) return bad;

			return ok;
		}
	} task;

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(NetBufferPool, QuotaPrioPass) {
	struct Task: public TestTask<Task> {
		Pool pool;
		Pool::Storage storage;

		bool run() {
			PoolJob jobs[4];
			pool.init(storage, 6, 6);

			jobs[0].start(pool, 6, Pool::Quota::None);
			Os::sleep(1);
			if(jobs[0].result == nullptr) return bad;

			jobs[1].start(pool, 6, Pool::Quota::Tx);
			Os::sleep(1);
			if(jobs[1].result != nullptr) return bad;

			jobs[2].start(pool, 6, Pool::Quota::Rx);
			Os::sleep(1);
			if(jobs[2].result != nullptr) return bad;

			jobs[0].destroy<Pool::Quota::None>(pool);
			if(jobs[1].result == nullptr) return bad;
			if(jobs[2].result != nullptr) return bad;

			jobs[1].destroy(pool);
			if(jobs[2].result == nullptr) return bad;

			return ok;
		}
	} task;

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}
