#ifndef MERGE_AND_SHRINK_MERGE_TREE_H
#define MERGE_AND_SHRINK_MERGE_TREE_H

#include <memory>
#include <utility>

namespace merge_and_shrink {
enum class UpdateOption {
    USE_FIRST,
    USE_SECOND,
    USE_RANDOM
};

class MergeTree {
    MergeTree *parent;
    MergeTree *left_child;
    MergeTree *right_child;
    int ts_index;

    bool is_leaf() const {
        return !left_child && !right_child;
    }

    bool has_two_leaf_children() const;
    MergeTree *get_left_most_sibling();
    std::pair<int, int> erase_children_and_set_index(int new_index);
    MergeTree *get_parent_of_ts_index(int index);
    MergeTree() = delete;
public:
    MergeTree(int ts_index);
    MergeTree(MergeTree *left_child, MergeTree *right_child);
    ~MergeTree();
    std::pair<int, int> get_next_merge(int new_index);
    void update(std::pair<int, int> merge, int new_index, UpdateOption option);
    int compute_num_internal_nodes() const;
    void postorder(int indentation) const;
};
}

#endif
