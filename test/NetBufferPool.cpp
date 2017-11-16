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
	typedef BufferPool<Os, 10, 10> Pool;

	class PoolJob: public Os::IoJob {
		static bool writeResult(typename Os::IoJob* item, typename Os::IoJob::Result result, const typename Os::IoJob::Reactivator &) {
			if(result == Os::IoJob::Result::Done) {
				static_cast<PoolJob*>(item)->result = reinterpret_cast<Pool::Block*>(item->param);
			}

			return false;
		}

		friend class Os::IoChannel;
		inline void prepare(size_t n) {
			result = nullptr;
			this->Os::IoJob::prepare(&PoolJob::writeResult, n);
		}

	public:
		Pool::Block* volatile result;
		inline PoolJob(): result(nullptr) {}
	};
};

TEST(NetBufferPool, Simple) {
	struct Task: public TestTask<Task> {
		bool error = false;
		Pool pool;

		void run() {
			pool.init();

			PoolJob jobs[3];
			pool.submit(jobs + 0, 3);

			Os::sleep(1);
			if(jobs[0].result == nullptr) {error = true; return;}

			pool.submit(jobs + 1, 4);

			Os::sleep(1);
			if(jobs[1].result == nullptr) {error = true; return;}

			pool.submit(jobs + 2, 5);

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

		void run() {
			PoolJob jobs[4];
			pool.init();

			pool.submit(jobs + 0, 5);
			Os::sleep(1);
			if(jobs[0].result == nullptr) {error = true; return;}

			pool.submit(jobs + 1, 5);
			Os::sleep(1);
			if(jobs[1].result == nullptr) {error = true; return;}

			pool.submit(jobs + 2, 6);
			Os::sleep(1);
			if(jobs[2].result != nullptr) {error = true; return;}

			pool.submit(jobs + 3, 4);
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

		void run() {
			PoolJob jobs[4];
			pool.init();

			pool.submit(jobs + 0, 5);
			Os::sleep(1);
			if(jobs[0].result == nullptr) {error = true; return;}

			pool.submit(jobs + 1, 5);
			Os::sleep(1);
			if(jobs[1].result == nullptr) {error = true; return;}

			pool.submit(jobs + 2, 6);
			Os::sleep(1);
			if(jobs[2].result != nullptr) {error = true; return;}

			pool.submit(jobs + 3, 4);
			Os::sleep(1);
			if(jobs[3].result != nullptr) {error = true; return;}

			pool.reclaim(jobs[0].result);
			Os::sleep(1);
			if(jobs[2].result != nullptr) {error = true; return;}
			if(jobs[3].result != nullptr) {error = true; return;}

			pool.cancel(jobs + 2);
			Os::sleep(1);
			if(jobs[3].result == nullptr) {error = true; return;}
		}
	} task;

	task.start();
	CommonTestUtils::start();
	CHECK(!task.error);
}
