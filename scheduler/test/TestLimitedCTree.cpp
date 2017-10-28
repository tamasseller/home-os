/*
 * TestLimitedCTree.cpp
 *
 *  Created on: 2017.10.28.
 *      Author: tooma
 */

#include "syscall/LimitedCTree.h"

#include "1test/Test.h"

#include <set>
#include <vector>
#include <random>
#include <utility>
#include <algorithm>
#include <iostream>

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

		template<class E = std::initializer_list<TestNode*>>
		void expect(int depth, const E& expected) {
			std::vector<TestNode*> order;

			CHECK(depth == traverse([this, &order](Node* at){
				order.push_back(static_cast<TestNode*>(getDual(at)));
			}));

			auto x = expected.begin();
			for(auto y: order) {
				CHECK(x != expected.end());
				CHECK(*x == y);
				++x;
			}

			CHECK(x == expected.end());
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

}

TEST_GROUP(LimitedCTreeInternal) {
	TestNode                                                         h='h';
	TestNode                         d='d',                                                          l='l';
	TestNode         b='b',                          f='f',                          j='j',                          n='n';
	TestNode a='a',          c='c',          e='e',          g='g',          i='i',          k='k',          m='m',          o='o';

	struct TestTree: TestTreeBase {
		template<class Self>
		TestTree(Self* self) {
			self->b.FirstNode::left = static_cast<FirstNode*>(&self->a);
			self->b.FirstNode::right = static_cast<FirstNode*>(&self->c);

			self->d.FirstNode::left = static_cast<FirstNode*>(&self->b);
			self->d.FirstNode::right = static_cast<FirstNode*>(&self->f);

			self->f.FirstNode::left = static_cast<FirstNode*>(&self->e);
			self->f.FirstNode::right = static_cast<FirstNode*>(&self->g);

			self->h.FirstNode::left = static_cast<FirstNode*>(&self->d);
			self->h.FirstNode::right = static_cast<FirstNode*>(&self->l);

			self->j.FirstNode::left = static_cast<FirstNode*>(&self->i);
			self->j.FirstNode::right = static_cast<FirstNode*>(&self->k);

			self->l.FirstNode::left = static_cast<FirstNode*>(&self->j);
			self->l.FirstNode::right = static_cast<FirstNode*>(&self->n);

			self->n.FirstNode::left = static_cast<FirstNode*>(&self->m);
			self->n.FirstNode::right = static_cast<FirstNode*>(&self->o);

			firstRoot = static_cast<FirstNode*>(&self->h);
		}
	} tree = this;
};

TEST(LimitedCTreeInternal, Simple) {
	tree.update();
	tree.expect(4, {&a, &b, &c, &d, &e, &f, &g, &h, &i, &j, &k, &l, &m, &n, &o});
}

TEST(LimitedCTreeInternal, TestCutA) {
	b.FirstNode::left = nullptr;
	tree.update();
	tree.expect(4, {&b, &c, &d, &e, &f, &g, &h, &i, &j, &k, &l, &m, &n, &o});
}

TEST(LimitedCTreeInternal, TestCutC) {
	b.FirstNode::right = nullptr;
	tree.update();
	tree.expect(4, {&a, &b, &d, &e, &f, &g, &h, &i, &j, &k, &l, &m, &n, &o});
}

TEST(LimitedCTreeInternal, TestCutE) {
	f.FirstNode::left = nullptr;
	tree.update();
	tree.expect(4, {&a, &b, &c, &d, &f, &g, &h, &i, &j, &k, &l, &m, &n, &o});
}

TEST(LimitedCTreeInternal, TestCutG) {
	f.FirstNode::right = nullptr;
	tree.update();
	tree.expect(4, {&a, &b, &c, &d, &e, &f, &h, &i, &j, &k, &l, &m, &n, &o});
}

TEST(LimitedCTreeInternal, TestCutI) {
	j.FirstNode::left = nullptr;
	tree.update();
	tree.expect(4, {&a, &b, &c, &d, &e, &f, &g, &h, &j, &k, &l, &m, &n, &o});
}

TEST(LimitedCTreeInternal, TestCutK) {
	j.FirstNode::right = nullptr;
	tree.update();
	tree.expect(4, {&a, &b, &c, &d, &e, &f, &g, &h, &i, &j, &l, &m, &n, &o});
}

TEST(LimitedCTreeInternal, TestCutM) {
	n.FirstNode::left = nullptr;
	tree.update();
	tree.expect(4, {&a, &b, &c, &d, &e, &f, &g, &h, &i, &j, &k, &l, &n, &o});
}

TEST(LimitedCTreeInternal, TestCutO) {
	n.FirstNode::right = nullptr;
	tree.update();
	tree.expect(4, {&a, &b, &c, &d, &e, &f, &g, &h, &i, &j, &k, &l, &m, &n});
}

TEST(LimitedCTreeInternal, TestCutB) {
	d.FirstNode::left = nullptr;
	tree.update();
	tree.expect(4, {&d, &e, &f, &g, &h, &i, &j, &k, &l, &m, &n, &o});
}

TEST(LimitedCTreeInternal, TestCutF) {
	d.FirstNode::right = nullptr;
	tree.update();
	tree.expect(4, {&a, &b, &c, &d, &h, &i, &j, &k, &l, &m, &n, &o});
}

TEST(LimitedCTreeInternal, TestCutJ) {
	l.FirstNode::left = nullptr;
	tree.update();
	tree.expect(4, {&a, &b, &c, &d, &e, &f, &g, &h, &l, &m, &n, &o});
}

TEST(LimitedCTreeInternal, TestCutN) {
	l.FirstNode::right = nullptr;
	tree.update();
	tree.expect(4, {&a, &b, &c, &d, &e, &f, &g, &h, &i, &j, &k, &l});
}

TEST(LimitedCTreeInternal, TestCutD) {
	h.FirstNode::left = nullptr;
	tree.update();
	tree.expect(4, {&h, &i, &j, &k, &l, &m, &n, &o});
}

TEST(LimitedCTreeInternal, TestCutL) {
	h.FirstNode::right = nullptr;
	tree.update();
	tree.expect(4, {&a, &b, &c, &d, &e, &f, &g, &h});
}

TEST(LimitedCTreeInternal, TestCutBGJO) {
	f.FirstNode::right = nullptr;
	d.FirstNode::left = nullptr;
	l.FirstNode::left = nullptr;
	n.FirstNode::right = nullptr;

	tree.update();
	tree.expect(3, {&d, &e, &f, &h, &l, &m, &n});
}

TEST(LimitedCTreeInternal, TestMultiple) {
	tree.update();
	tree.update();
	tree.expect(4, {&a, &b, &c, &d, &e, &f, &g, &h, &i, &j, &k, &l, &m, &n, &o});
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
	tree.expect(1, {&a});
	CHECK(tree.add<&compare>(&c));
	tree.expect(2, {&a, &c});
	CHECK(tree.add<&compare>(&b));
	tree.expect(2, {&a, &b, &c});
	tree.remove(&a);
	tree.expect(2, {&b, &c});
	tree.remove(&c);
	tree.expect(1, {&b});
	tree.remove(&b);
}

TEST(LimitedCTreePublic, Stress) {
	static constexpr auto n = 2048;

	std::vector<TestNode*> fresh;
	std::set<TestNode*, less> added;

	fresh.reserve(n);

	for(int i = 0; i < n; i++)
		fresh.push_back(new TestNode(i));

	std::shuffle(fresh.begin(), fresh.end(), std::mt19937(1));

	while(!fresh.empty()) {
		TestNode* node = fresh.back();
		fresh.pop_back();

		CHECK(tree.add<&compare>(node));
		added.insert(node);

		for(auto x: added) {
			CHECK(tree.contains<&compare>(x));
			CHECK(!tree.add<&compare>(x));
		}

		tree.expect((int)std::ceil(std::log2(added.size() + 1)), added);
	}
}
