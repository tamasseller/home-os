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

#include "syscall/LimitedCTree.h"

#include "1test/Test.h"

#include <utility>

namespace {
	struct TestNode: LimitedCTree::DualNode {
		const int c;
		TestNode(int c): c(c) {}
		TestNode(char c): c(c) {}
	};

	struct TestTreeBase: LimitedCTree {
		using LimitedCTree::Node;
		using LimitedCTree::DualNode;
		using LimitedCTree::FirstNode;
		using LimitedCTree::SecondNode;
		using LimitedCTree::update;

		template<class... C>
		void expect(int depth, C... c) {
			const char expected[] = {c...};
			const char * const end = expected + sizeof...(C);
			const char *p = expected;

			CHECK(depth == traverse([&](Node* at){
				CHECK(p != end);
				CHECK((static_cast<TestNode*>(getDual(at)))->c == *p++);
			}));
		}
	};

	static int compare(const LimitedCTree::DualNode* x, const LimitedCTree::DualNode* y) {
		return static_cast<const TestNode*>(x)->c - static_cast<const TestNode*>(y)->c;
	}

	struct less {
		bool operator()(TestNode* const & x, const TestNode* const & y) {
			return compare(x, y) < 0;
		}
	};

	TestNode                                                         h='h';
	TestNode                         d='d',                                                          l='l';
	TestNode         b='b',                          f='f',                          j='j',                          n='n';
	TestNode a='a',          c='c',          e='e',          g='g',          i='i',          k='k',          m='m',          o='o';

	struct TestTree: TestTreeBase {
		void init() {
			b.FirstNode::left = static_cast<FirstNode*>(&a);
			b.FirstNode::right = static_cast<FirstNode*>(&c);

			d.FirstNode::left = static_cast<FirstNode*>(&b);
			d.FirstNode::right = static_cast<FirstNode*>(&f);

			f.FirstNode::left = static_cast<FirstNode*>(&e);
			f.FirstNode::right = static_cast<FirstNode*>(&g);

			h.FirstNode::left = static_cast<FirstNode*>(&d);
			h.FirstNode::right = static_cast<FirstNode*>(&l);

			j.FirstNode::left = static_cast<FirstNode*>(&i);
			j.FirstNode::right = static_cast<FirstNode*>(&k);

			l.FirstNode::left = static_cast<FirstNode*>(&j);
			l.FirstNode::right = static_cast<FirstNode*>(&n);

			n.FirstNode::left = static_cast<FirstNode*>(&m);
			n.FirstNode::right = static_cast<FirstNode*>(&o);

			firstRoot = static_cast<FirstNode*>(&h);
			firstActive = true;
		}
	} tree;
}

TEST_GROUP(LimitedCTreeInternal) {
	TEST_SETUP() {
		tree.init();
	}
};

TEST(LimitedCTreeInternal, Simple) {
	tree.update();
	tree.expect(4, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o');
}

TEST(LimitedCTreeInternal, TestCutA) {
	b.FirstNode::left = nullptr;
	tree.update();
	tree.expect(4, 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o');
}

TEST(LimitedCTreeInternal, TestCutC) {
	b.FirstNode::right = nullptr;
	tree.update();
	tree.expect(4, 'a', 'b', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o');
}

TEST(LimitedCTreeInternal, TestCutE) {
	f.FirstNode::left = nullptr;
	tree.update();
	tree.expect(4, 'a', 'b', 'c', 'd', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o');
}

TEST(LimitedCTreeInternal, TestCutG) {
	f.FirstNode::right = nullptr;
	tree.update();
	tree.expect(4, 'a', 'b', 'c', 'd', 'e', 'f', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o');
}

TEST(LimitedCTreeInternal, TestCutI) {
	j.FirstNode::left = nullptr;
	tree.update();
	tree.expect(4, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'j', 'k', 'l', 'm', 'n', 'o');
}

TEST(LimitedCTreeInternal, TestCutK) {
	j.FirstNode::right = nullptr;
	tree.update();
	tree.expect(4, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'l', 'm', 'n', 'o');
}

TEST(LimitedCTreeInternal, TestCutM) {
	n.FirstNode::left = nullptr;
	tree.update();
	tree.expect(4, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'n', 'o');
}

TEST(LimitedCTreeInternal, TestCutO) {
	n.FirstNode::right = nullptr;
	tree.update();
	tree.expect(4, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n');
}

TEST(LimitedCTreeInternal, TestCutB) {
	d.FirstNode::left = nullptr;
	tree.update();
	tree.expect(4, 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o');
}

TEST(LimitedCTreeInternal, TestCutF) {
	d.FirstNode::right = nullptr;
	tree.update();
	tree.expect(4, 'a', 'b', 'c', 'd', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o');
}

TEST(LimitedCTreeInternal, TestCutJ) {
	l.FirstNode::left = nullptr;
	tree.update();
	tree.expect(4, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'l', 'm', 'n', 'o');
}

TEST(LimitedCTreeInternal, TestCutN) {
	l.FirstNode::right = nullptr;
	tree.update();
	tree.expect(4, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l');
}

TEST(LimitedCTreeInternal, TestCutD) {
	h.FirstNode::left = nullptr;
	tree.update();
	tree.expect(4, 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o');
}

TEST(LimitedCTreeInternal, TestCutL) {
	h.FirstNode::right = nullptr;
	tree.update();
	tree.expect(4, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h');
}

TEST(LimitedCTreeInternal, TestCutBGJO) {
	f.FirstNode::right = nullptr;
	d.FirstNode::left = nullptr;
	l.FirstNode::left = nullptr;
	n.FirstNode::right = nullptr;

	tree.update();
	tree.expect(3, 'd', 'e', 'f', 'h', 'l', 'm', 'n');
}

TEST(LimitedCTreeInternal, TestMultiple) {
	tree.update();
	tree.update();
	tree.expect(4, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o');
}

TEST_GROUP(LimitedCTreePublic) {
	static int compare(const LimitedCTree::DualNode* x, const LimitedCTree::DualNode* y) {
		return static_cast<const TestNode*>(x)->c - static_cast<const TestNode*>(y)->c;
	}

	TestTreeBase tree;
};

TEST(LimitedCTreePublic, Sanity) {
	TestNode n('n');
	CHECK(!tree.contains<&compare>(&n));

	CHECK(tree.add<&compare>(&n));

	CHECK(tree.contains<&compare>(&n));

	tree.remove(&n);

	CHECK(!tree.contains<&compare>(&n));
}

TEST(LimitedCTreePublic, Multiple) {
	TestNode a('a'), b('b'), c('c');

	CHECK(tree.add<&compare>(&a));
	tree.expect(1, 'a');
	CHECK(tree.add<&compare>(&c));
	tree.expect(2, 'a', 'c');
	CHECK(tree.add<&compare>(&b));
	tree.expect(2, 'a', 'b', 'c');
	tree.remove(&a);
	tree.expect(2, 'b', 'c');
	tree.remove(&c);
	tree.expect(1, 'b');
	tree.remove(&b);
}
