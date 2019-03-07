#ifndef CEGAR_REFINEMENT_HIERARCHY_H
#define CEGAR_REFINEMENT_HIERARCHY_H

#include "types.h"

#include <cassert>
#include <memory>
#include <ostream>
#include <utility>
#include <vector>

class AbstractTask;
class State;

namespace cegar {
class Node;

/*
  This class stores the refinement hierarchy of a Cartesian
  abstraction. The hierarchy forms a DAG with inner nodes for each
  split and leaf nodes for the abstract states.

  It is used for efficient lookup of abstract states during search.

  Inner nodes correspond to abstract states that have been split (or
  helper nodes, see below). Leaf nodes correspond to the current
  (unsplit) states in an abstraction. The use of helper nodes makes
  this structure a directed acyclic graph (instead of a tree).
*/
class RefinementHierarchy {
    std::shared_ptr<AbstractTask> task;
    std::vector<Node> nodes;

    NodeID add_node(int state_id);
    NodeID get_node_id(const State &state) const;

public:
    explicit RefinementHierarchy(const std::shared_ptr<AbstractTask> &task);

    /*
      Update the split tree for the new split. Additionally to the left
      and right child nodes add |values|-1 helper nodes that all have
      the right child as their right child and the next helper node as
      their left child.
    */
    std::pair<NodeID, NodeID> split(
        NodeID node_id, int var, const std::vector<int> &values,
        int left_state_id, int right_state_id);

    int get_abstract_state_id(const State &state) const;
};


class Node {
    /*
      While right_child is always the node of a (possibly split)
      abstract state, left_child may be a helper node. We add helper
      nodes to the hierarchy to allow for efficient lookup in case more
      than one fact is split off a state.
    */
    NodeID left_child;
    NodeID right_child;

    /* Before splitting the corresponding state for var and value, both
       members hold UNDEFINED. */
    int var;
    int value;

    // When splitting the corresponding state, we change this value to UNDEFINED.
    int state_id;

    bool information_is_valid() const;

public:
    explicit Node(int state_id);

    bool is_split() const;

    void split(int var, int value, NodeID left_child, NodeID right_child);

    int get_var() const {
        assert(is_split());
        return var;
    }

    NodeID get_child(int value) const {
        assert(is_split());
        if (value == this->value)
            return right_child;
        return left_child;
    }

    int get_state_id() const {
        assert(!is_split());
        return state_id;
    }

    friend std::ostream &operator<<(std::ostream &os, const Node &node);
};
}

#endif
