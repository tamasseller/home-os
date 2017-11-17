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

#include "SharedTable.h"

using Os = OsRr;

TEST_GROUP(NetSharedTable) {
    struct Kv {int k, v;};

    struct KvTable: private SharedTable<Os, Kv, 16> {
        inline typename SharedTable<Os, Kv, 16>::Entry* openOrCreate(int k) {
            return this->SharedTable<Os, Kv, 16>::findOrAllocate([k](Kv* e){ return e->k == k;});
        }

        inline bool set(int k, int v) {
            if(auto *x = openOrCreate(k)) {
                if(x->isValid()) {
                    x->release();
                    return false;
                }

                (*x)->k = k;
                (*x)->v = v;
                x->finalize();
                return true;
            }
        }

        inline int get(int k) {
            if(auto *x = this->SharedTable<Os, Kv, 16>::find([k](Kv* e){ return e->k == k;})) {
                int v = (*x)->v;
                x->release();
                return v;
            }

            return -1;
        }

        inline void remove(int k) {
            if(auto *x = this->SharedTable<Os, Kv, 16>::find([k](Kv* e){ return e->k == k;}))
                x->erase();
        }

    };
};


TEST(NetSharedTable, Simple)
{
    KvTable uut;

    CHECK(uut.get(1) == -1);
    uut.set(1, 2);
    CHECK(uut.get(1) == 2);
}

TEST(NetSharedTable, CreateCollision)
{
    KvTable uut;

    auto* x = uut.openOrCreate(1);
    auto* y = uut.openOrCreate(1);
    CHECK(x);
    CHECK(y);
    CHECK(x != y);
    CHECK(!x->isValid());
    CHECK(!y->isValid());

    (*x)->k = 1;
    x->finalize();

    auto* z = uut.openOrCreate(1);
    CHECK(z == x);
    CHECK(z->isValid());
}

TEST(NetSharedTable, DeleteReuse)
{
    KvTable uut;

    auto* x = uut.openOrCreate(1);
    (*x)->k = 1;
    x->finalize();

    auto* y = uut.openOrCreate(1);
    CHECK(y == x);
    y->erase();

    auto* z = uut.openOrCreate(2);
    CHECK(z == x);
}

TEST(NetSharedTable, OpenDeleteCollision)
{
    KvTable uut;

    auto* x = uut.openOrCreate(1);
    (*x)->k = 1;
    x->finalize();

    auto* y = uut.openOrCreate(1);

    auto* z = uut.openOrCreate(1);
    z->erase();

    auto* u = uut.openOrCreate(1);

    CHECK(u != y);

    auto* v = uut.openOrCreate(2);
    CHECK(v != y);
}
