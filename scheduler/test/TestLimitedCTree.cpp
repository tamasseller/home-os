/*
 * TestLimitedCTree.cpp
 *
 *  Created on: 2017.10.28.
 *      Author: tooma
 */

#include "syscall/LimitedCTree.h"

#include "1test/Test.h"

#include <vector>
#include <utility>

namespace {

	struct TestTreeBase: LimitedCTree {
		using LimitedCTree::Node;
		using LimitedCTree::DualNode;
		using LimitedCTree::FirstNode;
		using LimitedCTree::SecondNode;
		using LimitedCTree::update;

		struct TestNode: DualNode {
			const char c;
			TestNode(char c): c(c) {}
		};

		void expect(int depth, std::initializer_list<char>&& args) {
			const std::vector<char> expected(args);
			std::vector<char> order;

			CHECK(depth == traverse([this, &order](TestTreeBase::Node* at){
				order.push_back(static_cast<TestTreeBase::TestNode*>(getDual(at))->c);
			}));

			CHECK(order == expected);
		}
	};

}

TEST_GROUP(LimitedCTreeInternal) {
	struct TestTree: TestTreeBase {
		TestNode                                                         h='h';
		TestNode                         d='d',                                                          l='l';
		TestNode         b='b',                          f='f',                          j='j',                          n='n';
		TestNode a='a',          c='c',          e='e',          g='g',          i='i',          k='k',          m='m',          o='o';

		TestTree() {
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
		}
	} tree;
};

TEST(LimitedCTreeInternal, Simple) {
	tree.update();
	tree.expect(4, {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o'});
}

TEST(LimitedCTreeInternal, TestCutA) {
	tree.b.FirstNode::left = nullptr;
	tree.update();
	tree.expect(4, {'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o'});
}

TEST(LimitedCTreeInternal, TestCutC) {
	tree.b.FirstNode::right = nullptr;
	tree.update();
	tree.expect(4, {'a', 'b', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o'});
}

TEST(LimitedCTreeInternal, TestCutE) {
	tree.f.FirstNode::left = nullptr;
	tree.update();
	tree.expect(4, {'a', 'b', 'c', 'd', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o'});
}

TEST(LimitedCTreeInternal, TestCutG) {
	tree.f.FirstNode::right = nullptr;
	tree.update();
	tree.expect(4, {'a', 'b', 'c', 'd', 'e', 'f', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o'});
}

TEST(LimitedCTreeInternal, TestCutI) {
	tree.j.FirstNode::left = nullptr;
	tree.update();
	tree.expect(4, {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'j', 'k', 'l', 'm', 'n', 'o'});
}

TEST(LimitedCTreeInternal, TestCutK) {
	tree.j.FirstNode::right = nullptr;
	tree.update();
	tree.expect(4, {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'l', 'm', 'n', 'o'});
}

TEST(LimitedCTreeInternal, TestCutM) {
	tree.n.FirstNode::left = nullptr;
	tree.update();
	tree.expect(4, {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'n', 'o'});
}

TEST(LimitedCTreeInternal, TestCutO) {
	tree.n.FirstNode::right = nullptr;
	tree.update();
	tree.expect(4, {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n'});
}

TEST(LimitedCTreeInternal, TestCutB) {
	tree.d.FirstNode::left = nullptr;
	tree.update();
	tree.expect(4, {'d', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o'});
}

TEST(LimitedCTreeInternal, TestCutF) {
	tree.d.FirstNode::right = nullptr;
	tree.update();
	tree.expect(4, {'a', 'b', 'c', 'd', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o'});
}

TEST(LimitedCTreeInternal, TestCutJ) {
	tree.l.FirstNode::left = nullptr;
	tree.update();
	tree.expect(4, {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'l', 'm', 'n', 'o'});
}

TEST(LimitedCTreeInternal, TestCutN) {
	tree.l.FirstNode::right = nullptr;
	tree.update();
	tree.expect(4, {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l'});
}

TEST(LimitedCTreeInternal, TestCutD) {
	tree.h.FirstNode::left = nullptr;
	tree.update();
	tree.expect(4, {'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o'});
}

TEST(LimitedCTreeInternal, TestCutL) {
	tree.h.FirstNode::right = nullptr;
	tree.update();
	tree.expect(4, {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'});
}

TEST(LimitedCTreeInternal, TestCutBGJO) {
	tree.f.FirstNode::right = nullptr;
	tree.d.FirstNode::left = nullptr;
	tree.l.FirstNode::left = nullptr;
	tree.n.FirstNode::right = nullptr;

	tree.update();
	tree.expect(3, {'d', 'e', 'f', 'h', 'l', 'm', 'n'});
}

TEST(LimitedCTreeInternal, TestMultiple) {
	tree.update();
	tree.update();
	tree.expect(4, {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o'});
}

TEST_GROUP(LimitedCTreePublic) {
	struct TestNode: LimitedCTree::DualNode {
		const char c;
		TestNode(char c): c(c) {}

		static int compare(const LimitedCTree::DualNode* x, const LimitedCTree::DualNode* y) {
			return static_cast<const TestNode*>(x)->c - static_cast<const TestNode*>(y)->c;
		}
	};

	TestTreeBase tree;
};

TEST(LimitedCTreePublic, Sanity) {
	TestNode n('n');
	CHECK(!tree.contains<&TestNode::compare>(&n));

	CHECK(tree.add<&TestNode::compare>(&n));

	CHECK(tree.contains<&TestNode::compare>(&n));

	tree.remove(&n);

	CHECK(!tree.contains<&TestNode::compare>(&n));
}

TEST(LimitedCTreePublic, Multiple) {
	TestNode a('a'), b('b'), c('c');

	CHECK(tree.add<&TestNode::compare>(&a));
	tree.expect(1, {'a'});
	CHECK(tree.add<&TestNode::compare>(&c));
	tree.expect(2, {'a', 'c'});
	CHECK(tree.add<&TestNode::compare>(&b));
	tree.expect(2, {'a', 'b', 'c'});
	tree.remove(&a);
	tree.expect(2, {'b', 'c'});
	tree.remove(&c);
	tree.expect(1, {'b'});
	tree.remove(&b);
}
