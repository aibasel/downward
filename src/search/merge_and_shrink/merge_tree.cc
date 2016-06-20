#include "merge_tree.h"

#include "../utils/system.h"

#include <cassert>
#include <iostream>

using namespace std;

namespace merge_and_shrink {
MergeTree::MergeTree(int ts_index)
    : parent(nullptr),
      left_child(nullptr),
      right_child(nullptr),
      ts_index(ts_index) {
}

MergeTree::MergeTree(MergeTree *left_child, MergeTree *right_child)
    : parent(nullptr),
      left_child(left_child),
      right_child(right_child),
      ts_index(-1) {
    left_child->parent = this;
    right_child->parent = this;
}

MergeTree::~MergeTree() {
    delete left_child;
    delete right_child;
}

bool MergeTree::has_two_leaf_children() const {
    return left_child && right_child &&
        left_child->is_leaf() && right_child->is_leaf();
}

MergeTree *MergeTree::get_left_most_sibling() {
    if (has_two_leaf_children()) {
        return this;
    }

    if (left_child) {
        return left_child->get_left_most_sibling();
    }

    assert(right_child);
    return right_child->get_left_most_sibling();
}

pair<int, int> MergeTree::erase_children(int new_index) {
    assert(has_two_leaf_children());
    int left_child_index = left_child->ts_index;
    int right_child_index = right_child->ts_index;
    assert(left_child_index != -1);
    assert(right_child_index != -1);
    delete left_child;
    delete right_child;
    left_child = nullptr;
    right_child = nullptr;
    assert(ts_index == -1);
    ts_index = new_index;
    return make_pair(left_child_index, right_child_index);
}

pair<int, int> MergeTree::get_next_merge(int new_index) {
    MergeTree *next_merge = get_left_most_sibling();
    return next_merge->erase_children(new_index);
}

MergeTree *MergeTree::get_leaf_for_ts_index(int index) {
    if (ts_index == index) {
        return this;
    }

    if (left_child) {
        return left_child->get_leaf_for_ts_index(index);
    }

    if (right_child) {
        return right_child->get_leaf_for_ts_index(index);
    }

    ABORT("Could not find a parent of a leave with given index");
}

void MergeTree::update(pair<int, int> merge, int new_index, UpdateOption option) {
    assert(!parent); // only call on root tree
    int first_index = merge.first;
    int second_index = merge.second;
    MergeTree *first_leaf = get_leaf_for_ts_index(first_index);
    MergeTree *second_leaf = get_leaf_for_ts_index(second_index);

    if (first_leaf->parent == second_leaf->parent) {
        first_leaf->parent->erase_children(new_index);
    } else {
        if (option == UpdateOption::USE_FIRST) {
            first_leaf->ts_index = new_index;

            MergeTree *to_be_removed_node = second_leaf->parent;
            MergeTree *surviving_child = nullptr;
            if (to_be_removed_node->right_child == second_leaf) {
                surviving_child = to_be_removed_node->left_child;
            } else {
                assert(to_be_removed_node->left_child == second_leaf);
                surviving_child = to_be_removed_node->right_child;
            }
            surviving_child->parent = to_be_removed_node->parent;

            if (to_be_removed_node->parent->left_child == to_be_removed_node) {
                to_be_removed_node->parent->left_child = surviving_child;
            } else {
                assert(to_be_removed_node->parent->right_child == to_be_removed_node);
                to_be_removed_node->parent->right_child = surviving_child;
            }

            delete second_leaf->parent;
            second_leaf->parent = nullptr;
            delete second_leaf;
            second_leaf = nullptr;
        }
        // TODO: remaining options
    }
}

int MergeTree::compute_num_internal_nodes() const {
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

void MergeTree::postorder(int indentation) const {
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
}
