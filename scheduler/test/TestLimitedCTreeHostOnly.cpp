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

	struct TestTree: LimitedCTree {
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

		static int compare(const LimitedCTree::DualNode* x, const LimitedCTree::DualNode* y) {
			return static_cast<const TestNode*>(x)->c - static_cast<const TestNode*>(y)->c;
		}
	} tree;

	struct less {
		bool operator()(TestNode* const & x, const TestNode* const & y) {
			return TestTree::compare(x, y) < 0;
		}
	};
}

TEST(LimitedCTreeStress) {

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

		CHECK(tree.add<&TestTree::compare>(node));
		added.insert(node);

		for(auto x: added) {
			CHECK(tree.contains<&TestTree::compare>(x));
			CHECK(!tree.add<&TestTree::compare>(x));
		}

		tree.expect((int)std::ceil(std::log2(added.size() + 1)), added);
	}
}
