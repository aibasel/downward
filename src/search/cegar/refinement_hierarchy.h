#ifndef CEGAR_REFINEMENT_HIERARCHY_H
#define CEGAR_REFINEMENT_HIERARCHY_H

#include "types.h"

#include <cassert>
#include <memory>
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
    std::unique_ptr<Node> root;

    Node *get_node(const State &state) const;

public:
    explicit RefinementHierarchy(const std::shared_ptr<AbstractTask> &task);

    Node *get_root() const {
        return root.get();
    }

    int get_abstract_state_id(const State &state) const;
};


class Node {
    /*
      While right_child is always the node of a (possibly split)
      abstract state, left_child may be a helper node. We add helper
      nodes to the hierarchy to allow for efficient lookup in case more
      than one fact is split off a state.
    */
    // TODO: Use shared_ptr for left_child and unique_ptr for right_child?
    Node *left_child;
    Node *right_child;

    // Variable and value for which the corresponding state was split.
    int var;
    int value;

    // Holds UNDEFINED if this node belongs to a split state.
    int state_id;

public:
    explicit Node(int state_id);
    ~Node();

    Node(const Node &) = delete;
    Node &operator=(const Node &) = delete;

    /*
      Update the split tree for the new split. Additionally to the left
      and right child nodes add |values|-1 helper nodes that all have
      the right child as their right child and the next helper node as
      their left child.
    */
    std::pair<Node *, Node *> split(
        int var, const std::vector<int> &values, int left_state_id, int right_state_id);

    bool is_split() const {
        assert((!left_child && !right_child &&
                var == UNDEFINED && value == UNDEFINED && state_id != UNDEFINED) ||
               (left_child && right_child &&
                var != UNDEFINED && value != UNDEFINED && state_id == UNDEFINED));
        return left_child;
    }

    bool owns_right_child() const {
        assert(is_split());
        return !left_child->is_split() || left_child->right_child != right_child;
    }

    int get_var() const {
        assert(is_split());
        return var;
    }

    Node *get_child(int value) const;

    int get_state_id() const {
        assert(!is_split());
        assert(state_id != UNDEFINED);
        return state_id;
    }
};
}

#endif
