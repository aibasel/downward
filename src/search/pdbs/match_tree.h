#ifndef PDBS_MATCH_TREE_H
#define PDBS_MATCH_TREE_H

#include "types.h"

#include "pattern_database.h"

#include "../task_proxy.h"

#include <cstddef>
#include <vector>

namespace utils {
class LogProxy;
}

namespace pdbs {
class Projection;

/*
  Successor Generator for abstract operators.

  NOTE: MatchTree keeps a reference to the task proxy passed to the constructor.
  Therefore, users of the class must ensure that the task lives at least as long
  as the match tree.
*/

class MatchTree {
    TaskProxy task_proxy;
    Projection projection;
    struct Node;
    Node *root;
    void insert_recursive(int op_id,
                          const std::vector<FactPair> &regression_preconditions,
                          int pre_index,
                          Node **edge_from_parent);
    void get_applicable_operator_ids_recursive(
        Node *node, int state_index, std::vector<int> &operator_ids) const;
    void dump_recursive(Node *node, utils::LogProxy &log) const;
public:
    /*
      Initialize an empty match tree. We copy projection to ensure that the
      match tree remains in a valid state independently of projection.
    */
    MatchTree(const TaskProxy &task_proxy,
              const Projection &projection);
    ~MatchTree();
    /* Insert an abstract operator into the match tree, creating or
       enlarging it. */
    void insert(int op_id, const std::vector<FactPair> &regression_preconditions);

    /*
      Extracts all IDs of applicable abstract operators for the abstract state
      given by state_index (the index is converted back to variable/values
      pairs).
    */
    void get_applicable_operator_ids(
        int state_index, std::vector<int> &operator_ids) const;
    void dump(utils::LogProxy &log) const;
};
}

#endif
