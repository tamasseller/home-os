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
	typedef BufferPool<Os, 10, 1> Pool;

	class PoolJob: public Os::IoJob, Pool::IoData {
		static bool writeResult(typename Os::IoJob* item, typename Os::IoJob::Result result, typename Os::IoJob::Hook) {
			if(result == Os::IoJob::Result::Done) {
				auto self = static_cast<PoolJob*>(item);
				self->result = static_cast<Pool::Block*>(self->first);
			}

			return false;
		}

	public:
		inline bool start(Pool &pool, size_t n, Os::IoJob::Hook hook = nullptr) {
			result = nullptr;
			this->size = n;
			return submit(hook, &pool, &PoolJob::writeResult, this);
		}

		Pool::Block* volatile result;
		inline PoolJob(): result(nullptr) {}
	};
};

TEST(NetBufferPool, Simple) {
	struct Task: public TestTask<Task> {
		bool error = false;
		Pool pool;
		Pool::Storage storage;

		void run() {
			pool.init(storage);

			PoolJob jobs[3];
			jobs[0].start(pool, 3);

			Os::sleep(1);
			if(jobs[0].result == nullptr) {error = true; return;}

			jobs[1].start(pool, 4);

			Os::sleep(1);
			if(jobs[1].result == nullptr) {error = true; return;}

			jobs[2].start(pool, 5);

			Os::sleep(1);
			if(jobs[2].result != nullptr) {error = true; return;}

			pool.reclaim(jobs[0].result);

			Os::sleep(1);
			if(jobs[2].result == nullptr) {error = true; return;}
		}
	} task;

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(NetBufferPool, Ordering) {
	struct Task: public TestTask<Task> {
		bool error = false;
		Pool pool;
		Pool::Storage storage;

		void run() {
			PoolJob jobs[4];
			pool.init(storage);

			jobs[0].start(pool, 5);
			Os::sleep(1);
			if(jobs[0].result == nullptr) {error = true; return;}

			jobs[1].start(pool, 5);
			Os::sleep(1);
			if(jobs[1].result == nullptr) {error = true; return;}

			jobs[2].start(pool, 6);
			Os::sleep(1);
			if(jobs[2].result != nullptr) {error = true; return;}

			jobs[3].start(pool, 4);
			Os::sleep(1);
			if(jobs[3].result != nullptr) {error = true; return;}

			pool.reclaim(jobs[0].result);
			Os::sleep(1);
			if(jobs[2].result != nullptr) {error = true; return;}
			if(jobs[3].result != nullptr) {error = true; return;}

			pool.reclaim(jobs[1].result);
			Os::sleep(1);
			if(jobs[2].result == nullptr) {error = true; return;}
			if(jobs[3].result == nullptr) {error = true; return;}
		}
	} task;

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}

TEST(NetBufferPool, Cancelation) {
	struct Task: public TestTask<Task> {
		bool error = false;
		Pool pool;
		Pool::Storage storage;

		void run() {
			PoolJob jobs[4];
			pool.init(storage);

			jobs[0].start(pool, 5);
			Os::sleep(1);
			if(jobs[0].result == nullptr) {error = true; return;}

			jobs[1].start(pool, 5);
			Os::sleep(1);
			if(jobs[1].result == nullptr) {error = true; return;}

			jobs[2].start(pool, 6);
			Os::sleep(1);
			if(jobs[2].result != nullptr) {error = true; return;}

			jobs[3].start(pool, 4);
			Os::sleep(1);
			if(jobs[3].result != nullptr) {error = true; return;}

			pool.reclaim(jobs[0].result);
			Os::sleep(1);
			if(jobs[2].result != nullptr) {error = true; return;}
			if(jobs[3].result != nullptr) {error = true; return;}

			jobs[2].cancel();
			Os::sleep(1);
			if(jobs[3].result == nullptr) {error = true; return;}
		}
	} task;

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}
