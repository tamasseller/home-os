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

#ifndef LIMITEDCTREE_H_
#define LIMITEDCTREE_H_

/**
 * Limited concurrency, lock-free intrusive binary search tree
 * ===========================================================
 *
 * This container can be accessed for searching and updating
 * concurrently in scenarios where there is only one writer
 * and readers can preempt the writer only but not vice-versa.
 *
 * How it works
 * ------------
 *
 * This property is achieved by having a stable snapshot that
 * is atomically updated at the end of the update procedures.
 * Each node contains two set of left and right pointers and
 * no parent pointer. The tree contains two root pointers and
 * a selector field, which determines which one is active.
 * The first root only ever points to the first set of child
 * pointers and the second points to the second. Also child
 * pointers keep the same grouping, so there are two entirely
 * separate trees implemented in a single set of elements.
 *
 *            --------------------------
 *           | root1 | root2 | selector |
 *            --------------------------
 *      ______/             \__________
 *     /                               \
 *    -------------------     -------------------
 *   | l1 | r1 | l2 | r2 |   | l1 | r1 | l2 | r2 |
 *    -------------------     -------------------
 *           \         \  ______________/
 *            \         \/
 *             -------------------
 *            | l1 | r1 | l2 | r2 |
 *             -------------------
 *
 * Readers need to use the selector to determine which root
 * can be used as the starting point for traversal. The
 * writer uses the active root as a starting point for its
 * operations too, but only modifies the inactive root and
 * the inactive set of child pointers in the nodes. Once
 * it is done with building the updated tree it flips the
 * selector (which is atomic operation by nature) and the
 * readers see the new tree.
 *
 * Operations
 * ----------
 *
 * Lookup is done in the same way as for any binary search
 * tree, update operations are based on the Day-Stout-Warren
 * algorithm. The tree is normalized by flattening it into
 * a degenerate form, where all no node has left child, that
 * is the whole tree becomes a singly linked list, with the
 * right pointer pointing to the next element. Then this
 * structure (called a vine in the original paper) is
 * transformer into a fully balanced binary tree via regular
 * tree rotation operations.
 *
 * Concurrency
 * -----------
 *
 * Because only readers can preempt the writer, they can
 * only observe the same active root while searching the
 * tree. If a write operation is preempted, its changes
 * are invisible to the readers because they are carried
 * out on the hidden spare part of the nodes and the
 * inactive root.
 *
 * Characteristics
 * ---------------
 *
 * It has a O(n) time complexity for mutation and O(log(n))
 * for search, also it is optimally balanced in contrast with
 * common self-balancing (AVL, red-black, etc.) trees that
 * only approximate the optimal structure.
 *
 * The update operations require minimal, constant additional
 * space, allocated on the stack. The second, inactive set of
 * pointers are used as scratch area for various operations,
 * including building the new, updated tree structure.
 *
 * These properties make this container ideal for applications,
 * where updates are rare but lookups are frequent and can be
 * triggered by a source that is completely non-synchronized
 * with the writer.
 */
class LimitedCTree {
protected:
	/**
	 * A single pair of child nodes.
	 */
	struct Node {
		/**
		 * Left child or previous element.
		 *
		 * This field serves dual purpose, it can point to:
		 *
		 *  - the left (small) child in the tree form or
		 *  - the previous element temporarily, during update.
		 */
		Node *left = nullptr;

		/**
		 * Right child or next element.
		 *
		 * This field serves dual purpose, it can point to:
		 *
		 *  - the right (big) child in the tree form or
		 *  - the next element temporarily, during update.
		 */
		Node *right = nullptr;

		/**
		 * Add node as left neighbor.
		 *
		 * Adds the specified node as the left neighbor in
		 * a doubly linked fashion, updating both nodes
		 * pointers that should be facing each other.
		 */
		void addLeft(Node* n) {
			if(left) left->right = n;
			n->left = left;
			n->right = this;
			left = n;
		}

		/**
		 * Add node as right neighbor.
		 *
		 * Adds the specified node as the right neighbor
		 * in a doubly linked fashion, updating both nodes
		 * pointers that should be facing each other.
		 */
		void addRight(Node* n) {
			if(right) right->left = n;
			n->right = right;
			n->left = this;
			right = n;
		}

		/**
		 * Remove a node from the dual list.
		 *
		 * Removes the specified node by updating both
		 * neighboring nodes' pointers to each other.
		 */
		void remove() {
			if(right) right->left = left;
			if(left) left->right = right;
		}
	};

	/// First set of child pointers.
	struct FirstNode: Node {};

	/// Second set of child pointers.
	struct SecondNode: Node {};

public:

	/// The two sets of child pointers.
	struct DualNode: FirstNode, SecondNode {};

protected:

	/**
	 * Root of the first tree.
	 *
	 * This is the root of the tree formed by the
	 * first set of child pointers in the nodes.
	 *
	 * @NOTE Contains practically garbage when the
	 *       other root is active
	 */
	Node* volatile firstRoot = nullptr;

	/**
	 * Root of the second tree.
	 *
	 * This is the root of the tree formed by the
	 * second set of child pointers in the nodes.
	 *
	 * @NOTE Contains practically garbage when the
	 *       other root is active
	 */
	Node* volatile secondRoot = nullptr;

	/**
	 * Active tree selector.
	 *
	 * When this field is true, the _firstRoot_ is
	 * active and the _secondRoot_ is invalid. And
	 * the opposite is true otherwise.
	 */
	volatile bool firstActive = true;

	/**
	 * Get currently active root.
	 */
	Node* getRoot() {
		return firstActive ? firstRoot : secondRoot;
	}

	/**
	 * Swap trees.
	 *
	 * Update the inactive root and activate it.
	 */
	void swap(Node* newRoot)
	{
		if(firstActive)
			secondRoot = newRoot;
		else
			firstRoot = newRoot;

		firstActive = !firstActive;
	}

	/// Direction of translation
	enum Direction {
		RealToAlt,
		AltToReal
	};

	/**
	 * Get the other set of child pointers.
	 *
	 * Returns the active for the inactive _Node_
	 * of the same _DualNode_ or vice-versa.
	 */
	template<Direction state>
	Node* translate(Node* n) {
		if((state == RealToAlt && firstActive) || (state == AltToReal && !firstActive))
			return static_cast<SecondNode*>(static_cast<DualNode*>(static_cast<FirstNode*>(n)));
		else
			return static_cast<FirstNode*>(static_cast<DualNode*>(static_cast<SecondNode*>(n)));
	}

	/**
	 * Return the _DualNode_ for a _Node_ of the currently active set.
	 */
	DualNode* getDual(Node* n) {
		if(firstActive)
			return static_cast<DualNode*>(static_cast<FirstNode*>(n));
		else
			return static_cast<DualNode*>(static_cast<SecondNode*>(n));
	}

	/**
	 * Return the currently inactive _Node_ of the _DualNode_.
	 */
	Node* getTrans(DualNode* n) {
		if(firstActive)
			return static_cast<SecondNode*>(n);
		else
			return static_cast<FirstNode*>(n);
	}

	/**
	 * Bring the left node of an inactive _Node_ from the active
	 * domain and translate it to the currently inactive one.
	 */
	Node* transLeft(Node* alt) {
		return translate<RealToAlt>(translate<AltToReal>(alt)->left);
	}

	/**
	 * Bring the right node of an inactive _Node_ from the active
	 * domain and translate it to the currently inactive one.
	 */
	Node* transRight(Node* alt) {
		return translate<RealToAlt>(translate<AltToReal>(alt)->right);
	}

	/**
	 * Helper, linearizes the left subtree of the root.
	 */
	Node* goLeft(Node* it) {
		bool goingUp = false;

		while(true) {
			Node* right = transRight(it);
			if(!goingUp && right) {
					it->addRight(right);
					it = it->right;
			} else {
				Node* left = transLeft(it);
				if(left)  {
					it->addLeft(left);
					goingUp = false;
				} else
					goingUp = true;

				if(!it->left)
					return it;

				it = it->left;
			}
		}
	}

	/**
	 * Helper, linearizes the right subtree of the root.
	 */
	void goRight(Node* it)
	{
		bool goingUp = false;

		while(it) {
			Node* left = transLeft(it);
			if(!goingUp && left) {
					it->addLeft(left);
					it = it->left;
			} else {
				Node* right = transRight(it);
				if(right) {
					it->addRight(right);
					goingUp = false;
				} else
					goingUp = true;

				it = it->right;
			}
		}
	}

	/**
	 * Convert the active tree into doubly linked
	 * list of the respective inactive nodes.
	 */
	Node* linearize() {
		Node* altRoot = translate<RealToAlt>(getRoot());
		altRoot->right = altRoot->left = nullptr;

		if(transRight(altRoot)) {
			altRoot->addRight(transRight(altRoot));
			goRight(altRoot->right);
		}

		if(transLeft(altRoot)) {
			altRoot->addLeft(transLeft(altRoot));
			return goLeft(altRoot->left);
		} else
			return altRoot;
	}

	/**
	 * Convert the doubly linked list to a vine.
	 *
	 * Zeroes out the left pointers, resulting in degenerate tree form.
	 */
	void strip(Node* i) {
		for(; i; i = i->right)
			i->left = nullptr;
	}

	/**
	 * Perform left tree rotations, to make progress towards the balanced tree form.
	 */
	int compress(Node** back, unsigned int limit)
	{
		Node *i = *back;

		int n = 0;
		while(limit-- && i && i->right) {
			Node *newRoot = i->right;
			i->right = newRoot->left;
			newRoot->left = i;
			*back = newRoot;

			back = &newRoot->right;
			i = newRoot->right;

			n++;
		}

		return n;
	}

	/**
	 * Convert the vine form into a fully balanced tree.
	 */
	Node* vineToTree(Node* smallest)
	{
		Node* ret = smallest;

		int x = compress(&ret, -1u);

		while(x >>= 1)
			compress(&ret, x);

		return ret;
	}

	/**
	 * Do a full rebuild without modification.
	 *
	 * @NOTE This method is used for testing only.
	 */
	void update()
	{
		Node* smallest = linearize();
		strip(smallest);
		swap(vineToTree(smallest));
	}

	/**
	 * In-order traversal.
	 *
	 * Call the visitor for each node in order. Also
	 * calculates the maximal depth.
	 *
	 * @NOTE This method is used for testing only.
	 */
	template<class Visitor>
	int traverse(Visitor&& visitor, Node* at = nullptr)
	{
		int depthLeft = 0, depthRight = 0;

		if(!at) at = getRoot();

		if(at->left)
			 depthLeft = traverse(visitor, at->left);

		visitor(at);

		if(at->right)
			depthRight = traverse(visitor, at->right);

		return ((depthRight > depthLeft) ? depthRight : depthLeft) + 1;
	}


public:
	template<int (*compare)(const DualNode*, const DualNode*)>
	bool add(DualNode* dnode) {
		Node* node = getTrans(dnode);

		if(Node* parent = getRoot()) {
			bool isLeft;

			while(true) {
				if(int cmp = compare(dnode, getDual(parent))) {
					isLeft = (cmp < 0);
					if(Node *next = isLeft ? parent->left : parent->right)
						parent = next;
					else
						break;
				} else
					return false;
			}

			parent = translate<RealToAlt>(parent);
			Node* smallest = linearize();

			if(isLeft) {
				parent->addLeft(node);

				if(parent == smallest)
					smallest = node;
			} else
				parent->addRight(node);

			strip(smallest);
			swap(vineToTree(smallest));
		} else
			swap(node);

		return true;
	}

	void remove(DualNode* dnode) {
		Node* node = getTrans(dnode);
		Node* smallest = linearize();

		if(node == smallest)
			smallest = smallest->right;

		node->remove();
		strip(smallest);
		swap(vineToTree(smallest));
	}

	template<int (*compare)(const DualNode*, const DualNode*)>
	bool contains(DualNode* dnode)
	{
		for(Node* x = getRoot(); x; ) {
			if(int cmp = compare(dnode, getDual(x)))
				x = (cmp < 0) ? x->left : x->right;
			else
				return true;
		}

		return false;
	}
};

#endif /* LIMITEDCTREE_H_ */
