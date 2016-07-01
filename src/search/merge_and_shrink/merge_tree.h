#ifndef MERGE_AND_SHRINK_MERGE_TREE_H
#define MERGE_AND_SHRINK_MERGE_TREE_H

#include <memory>
#include <utility>

namespace utils {
class RandomNumberGenerator;
}

namespace merge_and_shrink {
extern const int UNINITIALIZED;

struct MergeTreeNode {
    MergeTreeNode *parent;
    MergeTreeNode *left_child;
    MergeTreeNode *right_child;
    int ts_index;

    MergeTreeNode() = delete;
    MergeTreeNode(int ts_index);
    MergeTreeNode(MergeTreeNode *left_child, MergeTreeNode*right_child);
    ~MergeTreeNode();

    MergeTreeNode *get_left_most_sibling();
    std::pair<int, int> erase_children_and_set_index(int new_index);
    MergeTreeNode *get_parent_of_ts_index(int index);
    int compute_num_internal_nodes() const;
    void postorder(int indentation) const;

    bool is_leaf() const {
        return !left_child && !right_child;
    }

    bool has_two_leaf_children() const {
        return left_child && right_child &&
            left_child->is_leaf() && right_child->is_leaf();
    }
};

enum class UpdateOption {
    USE_FIRST,
    USE_SECOND,
    USE_RANDOM
};

class MergeTree {
    MergeTreeNode *root;
    std::shared_ptr<utils::RandomNumberGenerator> rng;
    MergeTree() = delete;
public:
    MergeTree(
        MergeTreeNode *root,
        std::shared_ptr<utils::RandomNumberGenerator> rng);
    ~MergeTree();
    std::pair<int, int> get_next_merge(int new_index);
    void update(std::pair<int, int> merge, int new_index, UpdateOption option);

    bool done() const {
        return root->is_leaf();
    }

    int compute_num_internale_nodes() const {
        return root->compute_num_internal_nodes();
    }

    void postorder(int indentation) const {
        return root->postorder(indentation);
    }
};
}

#endif
