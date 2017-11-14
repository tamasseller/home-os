/*
 * NetBufferPool.cpp
 *
 *  Created on: 2017.11.13.
 *      Author: tooma
 */

#include "common/CommonTestUtils.h"

#include "BufferPool.h"

using Os = OsRr;

TEST_GROUP(NetBufferPool) {
	typedef BufferPool<Os, 10, 10> Pool;
};

TEST(NetBufferPool, Simple) {
	Pool pool;

	auto x = pool.allocate(3);
}
