#ifndef CEGAR_SPLIT_TREE_H
#define CEGAR_SPLIT_TREE_H

#include "utils.h"

#include <cassert>
#include <utility>
#include <vector>

namespace cegar {
class Node {
    // Forbid copy constructor and copy assignment operator.
    Node(const Node &);
    Node &operator=(const Node &);

    // While right_child always corresponds to a (possibly split) abstract
    // state, left_child may correspond to a helper node. Helper nodes
    // are added to the hierarchy to allow for efficient lookup in case
    // more than one atom is split off a state.
    Node *left_child;
    Node *right_child;
    // Save the variable and value for which abs_state was refined
    // and the resulting abstract child states.
    int var;
    int value;
    // Estimated cost to goal node.
    int h;

public:
    explicit Node();
    // Update the split tree for the new split.
    // Additionally to the two normal child nodes Node(left) and Node(right)
    // add |values|-1 OR-nodes that all have Node(right) as their right child
    // and the next OR-node as their left child.
    std::pair<Node *, Node *> split(int var, const std::vector<int> &values);

    bool is_split() const {
        assert((left_child == 0 && right_child == 0 &&
                var == UNDEFINED && value == UNDEFINED) ||
               (left_child != 0 && right_child != 0 &&
                var != UNDEFINED && value != UNDEFINED));
        return var != UNDEFINED;
    }

    int get_var() const {
        assert(is_split());
        return var;
    }

    //int get_value() const {return value; }
    Node *get_child(int value) const;
    Node *get_left_child() const;
    Node *get_right_child() const;

    void set_h(int new_h) {
        assert(new_h >= h);
        h = new_h;
    }
    int get_h() const {return h; }
};

class SplitTree {
    Node *root;

public:
    SplitTree();
    Node *get_node(const State &state) const;
    Node *get_root() const {return root; }
};
}

#endif
