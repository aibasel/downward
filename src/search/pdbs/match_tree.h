#ifndef PDBS_MATCH_TREE_H
#define PDBS_MATCH_TREE_H

#include "../task_proxy.h"

#include <cstddef>
#include <memory>
#include <vector>

class AbstractOperator;

// Successor Generator for abstract operators.
class MatchTree {
    const std::shared_ptr<AbstractTask> task;
    TaskProxy task_proxy;
    struct Node;
    // See PatternDatabase for documentation on pattern and hash_multipliers.
    std::vector<int> pattern;
    std::vector<size_t> hash_multipliers;
    Node *root;
    void insert_recursive(const AbstractOperator &op,
                          int pre_index,
                          Node **edge_from_parent);
    void get_applicable_operators_recursive(
        Node *node, const size_t state_index,
        std::vector<const AbstractOperator *> &applicable_operators) const;
    void dump_recursive(Node *node) const;
public:
    // Initialize an empty match tree.
    MatchTree(const std::shared_ptr<AbstractTask> task,
              const std::vector<int> &pattern,
              const std::vector<size_t> &hash_multipliers);
    ~MatchTree();
    /* Insert an abstract operator into the match tree, creating or
       enlarging it. */
    void insert(const AbstractOperator &op);

    /*
      Extracts all applicable abstract operators for the abstract state given
      by state_index (the index is converted back to variable/values pairs).
    */
    void get_applicable_operators(
        size_t state_index,
        std::vector<const AbstractOperator *> &applicable_operators) const;
    void dump() const;
};

#endif
