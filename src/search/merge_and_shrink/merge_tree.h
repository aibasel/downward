#ifndef MERGE_AND_SHRINK_MERGE_TREE_H
#define MERGE_AND_SHRINK_MERGE_TREE_H

#include <memory>
#include <utility>

namespace utils {
class RandomNumberGenerator;
}

namespace merge_and_shrink {
extern const int UNINITIALIZED;

/*
  Binary tree data structure with convenience methods for removing
  sibling leaf nodes and modifying the tree by removing specific
  leaf nodes that are not siblings (see also MergeTree class).
 */
struct MergeTreeNode {
    MergeTreeNode *parent;
    MergeTreeNode *left_child;
    MergeTreeNode *right_child;
    int ts_index;

    MergeTreeNode() = delete;
    // Copy constructor. Does not set parent pointers.
    MergeTreeNode(const MergeTreeNode &other);
    MergeTreeNode(int ts_index);
    MergeTreeNode(MergeTreeNode *left_child, MergeTreeNode *right_child);
    ~MergeTreeNode();

    MergeTreeNode *get_left_most_sibling();
    std::pair<int, int> erase_children_and_set_index(int new_index);
    // Find the parent node for the given index.
    MergeTreeNode *get_parent_of_ts_index(int index);
    int compute_num_internal_nodes() const;
    void inorder(int offset, int current_indentation) const;

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

/*
  This class manages a binary tree data structure (MergeTreeNode) that
  represents a merge tree.

  In the common use case, it the merge tree is used as "the merge strategy"
  and hence it is always synchronized with the current factored transition
  system managed by the merge-and-shrink heuristic. In that case, when asked
  for a next merge, the *left-most* sibling leaf pair is returned and their
  parent node updated to represent the resulting composite transition system.

  NOTE: returning the left-most sibling leaf pair does not allow to represent
  arbitrary merge strategies with this class, because there is not possibility
  to specify the merge order of current sibling leaf nodes in an arbitrary
  way. For existing precomputed merge strategies like the linear ones or MIASM,
  this does not matter.

  For the less common use case of using a merge tree within another merge
  strategy where the merge tree acts as a fallback mechanism, the merge tree
  has to be kept synchronized with the factored transition system. This
  requires informing the merge tree about all merges that happen and that may
  differ from what the merge tree prescribes. The method update provides this
  functionality, using the user specified choice update_option to choose one
  of two possible leaf nodes representing the indices of the given merge as the
  future node representing the merge.
*/
class MergeTree {
    MergeTreeNode *root;
    std::shared_ptr<utils::RandomNumberGenerator> rng;
    UpdateOption update_option;
    /*
      Find the two parents (can be the same) of the given indices. The first
      one will correspond to a merge that would have been merged earlier in
      the merge tree than the second one.
    */
    std::pair<MergeTreeNode *, MergeTreeNode *> get_parents_of_ts_indices(
        const std::pair<int, int> &ts_indices, int new_index);
    MergeTree() = delete;
public:
    MergeTree(
        MergeTreeNode *root,
        const std::shared_ptr<utils::RandomNumberGenerator> &rng,
        UpdateOption update_option);
    ~MergeTree();
    std::pair<int, int> get_next_merge(int new_index);
    /*
      Inform the merge tree about a merge that happened independently of
      using the tree's method get_next_merge.
    */
    void update(std::pair<int, int> merge, int new_index);

    bool done() const {
        return root->is_leaf();
    }

    int compute_num_internal_nodes() const {
        return root->compute_num_internal_nodes();
    }

    // NOTE: this performs the "inverted" inorder_traversal, i.e. from right
    // to left, so that the printed tree matches the correct left-to-right
    // order.
    void inorder_traversal(int indentation_offset) const;
};
}

#endif
