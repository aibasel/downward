#ifndef PDBS_MATCH_TREE_H
#define PDBS_MATCH_TREE_H

#include <cstddef>
#include <vector>

// Implements a Successor Generator for abstract operators
class AbstractOperator;
class MatchTree {
    struct Node;
    std::vector<int> pattern; // as in PDBHeuristic
    std::vector<size_t> hash_multipliers; // as in PDBHeuristic
    Node *root;
    void build_recursively(const AbstractOperator &op, int pre_index, Node **edge_from_parent); // called by insert
    void traverse(Node *node, const size_t state_index,
                  std::vector<const AbstractOperator *> &applicable_operators) const; // called by get_applicable_operators
    void _dump(Node *node) const;
public:
    MatchTree(const std::vector<int> &pattern, const std::vector<size_t> &hash_multipliers); // initialize empty match tree
    ~MatchTree();
    void insert(const AbstractOperator &op); // inserts an abstract operator into the match tree, creating or enlarging it

    // Extracts all applicable abstract operators for the abstract state given by state_index (the index is converted back
    // to variable/values pairs)
    void get_applicable_operators(size_t state_index,
                                  std::vector<const AbstractOperator *> &applicable_operators) const;
    void dump() const;
};

#endif
