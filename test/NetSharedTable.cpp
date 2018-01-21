/*
 * NetSharedTable.cpp
 *
 *      Author: tamas.seller
 */

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

#include "core/SharedTable.h"

using Os = OsRr;

namespace {
	int ctorDtorCount;
}

TEST_GROUP(NetSharedTable) {
    struct Kv {
    	int k, v;
    	inline Kv() {ctorDtorCount++;}
    	inline ~Kv() {ctorDtorCount--;}
    };

    struct KvTable: SharedTable<Os, Kv, 16> {
        inline typename SharedTable<Os, Kv, 16>::Entry* openOrCreate(int k) {
            auto ret = this->SharedTable<Os, Kv, 16>::findOrAllocate([k](Kv* e){ return e->k == k;});

            if(!ret->isValid())
            	ret->init();

            return ret;
        }

        inline bool set(int k, int v) {
            if(auto *x = openOrCreate(k)) {
                if(x->isValid()) {
                    x->release();
                    return false;
                }

                x->access().k = k;
                x->access().v = v;
                x->finalize();
                return true;
            }
        }

        inline int get(int k) {
            if(auto *x = this->SharedTable<Os, Kv, 16>::find([k](Kv* e){ return e->k == k;})) {
                int v = x->access().v;
                x->release();
                return v;
            }

            return -1;
        }

        inline void remove(int k) {
            if(auto *x = this->SharedTable<Os, Kv, 16>::find([k](Kv* e){ return e->k == k;}))
                x->erase();
        }

        inline int highestKey() {
            int ret = -1;

            if(auto *x = this->SharedTable<Os, Kv, 16>::findBest([](Kv* e){ return e->k;})) {
                ret = x->access().k;
                x->release();
            }

            return ret;
        }
    };
};


TEST(NetSharedTable, Simple)
{
    KvTable uut;

    ctorDtorCount = 0;

    CHECK(uut.get(1) == -1);

    CHECK(ctorDtorCount == 0);

    uut.set(1, 2);

    CHECK(ctorDtorCount == 1);

    CHECK(uut.get(1) == 2);

    ctorDtorCount = 0;
}

TEST(NetSharedTable, ReleaseUnused)
{
    KvTable uut;

    ctorDtorCount = 0;

    auto x = uut.openOrCreate(1);

    CHECK(ctorDtorCount == 1);

    x->release();

    CHECK(ctorDtorCount == 0);
}

TEST(NetSharedTable, FindAfterEmpty)
{
    KvTable uut;

    ctorDtorCount = 0;

    auto x = uut.openOrCreate(1);
    uut.set(2, 345);
    uut.set(3, 456);

    CHECK(ctorDtorCount == 3);

    x->release();

    CHECK(ctorDtorCount == 2);

    CHECK(uut.get(3) == 456);

    CHECK(ctorDtorCount == 2);

    auto z = uut.openOrCreate(2);
    CHECK(z->access().v == 345);
    z->erase();

    CHECK(ctorDtorCount == 1);
}

TEST(NetSharedTable, CreateCollision)
{
    KvTable uut;
    ctorDtorCount = 0;

    auto* x = uut.openOrCreate(1);
    CHECK(ctorDtorCount == 1);
    auto* y = uut.openOrCreate(1);
    CHECK(ctorDtorCount == 2);
    CHECK(x);
    CHECK(y);
    CHECK(x != y);
    CHECK(!x->isValid());
    CHECK(!y->isValid());

    x->access().k = 1;
    x->finalize();

    auto* z = uut.openOrCreate(1);
    CHECK(ctorDtorCount == 2);
    CHECK(z == x);
    CHECK(z->isValid());
}

TEST(NetSharedTable, DeleteReuse)
{
    KvTable uut;
    ctorDtorCount = 0;

    auto* x = uut.openOrCreate(1);
    CHECK(ctorDtorCount == 1);
    x->access().k = 1;
    x->finalize();

    auto* y = uut.openOrCreate(1);
    CHECK(ctorDtorCount == 1);
    CHECK(y == x);
    y->erase();

    CHECK(ctorDtorCount == 0);

    auto* z = uut.openOrCreate(2);
    CHECK(z == x);

    CHECK(ctorDtorCount == 1);
}

TEST(NetSharedTable, OpenDeleteCollision)
{
    KvTable uut;
    ctorDtorCount = 0;

    auto* x = uut.openOrCreate(1);
    CHECK(ctorDtorCount == 1);
    x->access().k = 1;
    x->finalize();

    auto* y = uut.openOrCreate(1);
    CHECK(ctorDtorCount == 1);

    auto* z = uut.openOrCreate(1);
    z->erase();
    CHECK(ctorDtorCount == 1);

    auto* u = uut.openOrCreate(1);
    CHECK(ctorDtorCount == 2);
    CHECK(u != y);

    auto* v = uut.openOrCreate(2);
    CHECK(ctorDtorCount == 3);
    CHECK(v != y);
}

TEST(NetSharedTable, FindBest)
{
    KvTable uut;
    ctorDtorCount = 0;

    CHECK(uut.highestKey() == -1);

    uut.set(2, 1);
    CHECK(ctorDtorCount == 1);

    CHECK(uut.highestKey() == 2);

    uut.set(1, 1);
    CHECK(ctorDtorCount == 2);

    CHECK(uut.highestKey() == 2);

    uut.remove(2);
    CHECK(ctorDtorCount == 1);

    CHECK(uut.highestKey() == 1);

    uut.set(0, 1);
    CHECK(ctorDtorCount == 2);

    CHECK(uut.highestKey() == 1);

    uut.set(3, 1);
    CHECK(ctorDtorCount == 3);

    CHECK(uut.highestKey() == 3);
}

TEST(NetSharedTable, Iterate)
{
    KvTable uut;
    ctorDtorCount = 0;

    uut.set(1, 1);
    uut.set(2, 2);
    uut.set(3, 3);
    uut.set(4, 1);
    uut.set(5, 2);
    uut.set(6, 3);
    uut.set(7, 1);
    uut.set(8, 2);

    size_t base = 0;
    auto x = uut.find([](const Kv* kv){return kv->v == 2;}, base);
    CHECK(x->access().k == 2);
    CHECK(base == 2);

    auto y = uut.find([](const Kv* kv){return kv->v == 2;}, base);
    CHECK(y->access().k == 5);
    CHECK(base == 5);

    auto z = uut.find([](const Kv* kv){return kv->v == 2;}, base);
    CHECK(z->access().k == 8);
    CHECK(base == 8);

    auto u = uut.find([](const Kv* kv){return kv->v == 2;}, base);
    CHECK(!u);
    CHECK(base == 8);
}
