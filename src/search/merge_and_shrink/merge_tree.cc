#include "merge_tree.h"

#include "../utils/rng.h"
#include "../utils/system.h"

#include <cassert>
#include <iostream>

using namespace std;

namespace merge_and_shrink {
const int UNINITIALIZED = -1;

MergeTreeNode::MergeTreeNode(int ts_index)
    : parent(nullptr),
      left_child(nullptr),
      right_child(nullptr),
      ts_index(ts_index) {
}

MergeTreeNode::MergeTreeNode(
    MergeTreeNode *left_child,
    MergeTreeNode *right_child)
    : parent(nullptr),
      left_child(left_child),
      right_child(right_child),
      ts_index(UNINITIALIZED) {
    left_child->parent = this;
    right_child->parent = this;
}

MergeTreeNode::~MergeTreeNode() {
    delete left_child;
    delete right_child;
    left_child = nullptr;
    right_child = nullptr;
}

MergeTreeNode *MergeTreeNode::get_left_most_sibling() {
    if (has_two_leaf_children()) {
        return this;
    }

    if (left_child) {
        return left_child->get_left_most_sibling();
    }

    assert(right_child);
    return right_child->get_left_most_sibling();
}

pair<int, int> MergeTreeNode::erase_children_and_set_index(int new_index) {
    assert(has_two_leaf_children());
    int left_child_index = left_child->ts_index;
    int right_child_index = right_child->ts_index;
    assert(left_child_index != UNINITIALIZED);
    assert(right_child_index != UNINITIALIZED);
    delete left_child;
    delete right_child;
    left_child = nullptr;
    right_child = nullptr;
    assert(ts_index == UNINITIALIZED);
    ts_index = new_index;
    return make_pair(left_child_index, right_child_index);
}

MergeTreeNode *MergeTreeNode::get_parent_of_ts_index(int index) {
    if ((left_child && left_child->ts_index == index) &&
        (right_child && right_child->ts_index == index)) {
        return this;
    }

    if (left_child) {
        return left_child->get_parent_of_ts_index(index);
    }

    assert(right_child);
    return right_child->get_parent_of_ts_index(index);
}

int MergeTreeNode::compute_num_internal_nodes() const {
    if (is_leaf()) {
        return 0;
    } else {
        int number_of_internal_nodes = 1; // count the node itself
        if (left_child) {
            number_of_internal_nodes += left_child->compute_num_internal_nodes();
        }
        if (right_child) {
            number_of_internal_nodes += right_child->compute_num_internal_nodes();
        }
        return number_of_internal_nodes;
    }
}

void MergeTreeNode::postorder(int indentation) const {
    if (left_child) {
        left_child->postorder(indentation + 1);
    }
    if (right_child) {
        right_child->postorder(indentation + 1);
    }
    for (int i = 0; i < indentation; ++i) {
        cout << "  ";
    }
    cout << ts_index << endl;
}

MergeTree::MergeTree(
    MergeTreeNode *root,
    shared_ptr<utils::RandomNumberGenerator> rng)
    : root(root), rng(rng) {
}

MergeTree::~MergeTree() {
    delete root;
    root = nullptr;
}

pair<int, int> MergeTree::get_next_merge(int new_index) {
    MergeTreeNode *next_merge = root->get_left_most_sibling();
    return next_merge->erase_children_and_set_index(new_index);
}

void MergeTree::update(pair<int, int> merge, int new_index, UpdateOption option) {
    int first_index = merge.first;
    int second_index = merge.second;
    MergeTreeNode *first_parent = root->get_parent_of_ts_index(first_index);
    MergeTreeNode *second_parent = root->get_parent_of_ts_index(second_index);

    if (first_parent == second_parent) { // given merge already in the tree
        first_parent->erase_children_and_set_index(new_index);
    } else {
        MergeTreeNode *surviving_parent = nullptr;
        MergeTreeNode *to_be_removed_parent = nullptr;
        if (option == UpdateOption::USE_FIRST) {
            surviving_parent = first_parent;
            to_be_removed_parent = second_parent;
        } else if (option == UpdateOption::USE_SECOND) {
            surviving_parent = second_parent;
            to_be_removed_parent = first_parent;
        } else if (option == UpdateOption::USE_RANDOM) {
            int random = (*rng)(2);
            surviving_parent = (random == 0 ? first_parent : second_parent);
            to_be_removed_parent = (random == 0 ? second_parent : first_parent);
        } else {
            ABORT("Unknown merge tree update option");
        }

        // Update the leaf node corresponding to one of the indices to
        // correspond to the merged index.
        if (surviving_parent->left_child->ts_index == first_index ||
            surviving_parent->left_child->ts_index == second_index) {
            surviving_parent->left_child->ts_index = new_index;
        } else {
            assert(surviving_parent->right_child->ts_index == first_index ||
                surviving_parent->right_child->ts_index == second_index);
            surviving_parent->right_child->ts_index = new_index;
        }

        // Remove the other leaf and its parent from the tree, moving up
        // the other child of the removed parent.
        to_be_removed_parent->ts_index = -2; // mark the node
        MergeTreeNode *to_be_removed_parents_parent = to_be_removed_parent->parent;
        bool is_right_child = to_be_removed_parents_parent->right_child->ts_index == -2;
        if (to_be_removed_parent->left_child->ts_index == first_index ||
            to_be_removed_parent->left_child->ts_index == second_index) {
            if (is_right_child) {
                to_be_removed_parents_parent->right_child = move(to_be_removed_parent->left_child);
            } else {
                to_be_removed_parents_parent->left_child = move(to_be_removed_parent->left_child);
            }
            to_be_removed_parent->left_child->parent = to_be_removed_parents_parent;
        } else {
            assert(to_be_removed_parent->right_child->ts_index == first_index ||
                   to_be_removed_parent->right_child->ts_index == second_index);
            if (is_right_child) {
                to_be_removed_parents_parent->right_child = move(to_be_removed_parent->right_child);
            } else {
                to_be_removed_parents_parent->left_child = move(to_be_removed_parent->right_child);
            }
            to_be_removed_parent->right_child->parent = to_be_removed_parents_parent;
        }
        to_be_removed_parent = nullptr;
    }
}
}
