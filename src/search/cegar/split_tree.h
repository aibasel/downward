#ifndef CEGAR_SPLIT_TREE_H
#define CEGAR_SPLIT_TREE_H

#include "utils.h"
#include "../state.h"

#include <vector>

namespace cegar_heuristic {
class AbstractState;

class Node {
private:
    // Forbid copy constructor and copy assignment operator.
    Node(const Node &);
    Node &operator=(const Node &);

    AbstractState *abs_state;
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
    explicit Node(AbstractState *state = 0);
    // Update the split tree for the new split of "abs_state" into "left" and
    // "right".
    // Additionally to the two normal child nodes Node(left) and Node(right)
    // add |values|-1 OR-nodes that all have Node(right) as their right child
    // and the next OR-node as their left child.
    void split(int var, const std::vector<int> &values, AbstractState *left, AbstractState *right);

    bool is_split() const {return var != UNDEFINED; }
    int get_var() const {return var; }
    Node *get_child(int value) const;
    AbstractState *get_left_child_state() const;
    AbstractState *get_right_child_state() const;
    AbstractState *get_abs_state() const {return abs_state; }
    void set_h(int h) {
        assert(abs_state);
        assert(h >= this->h);
        this->h = h;
    }
    int get_h() const {return h; }
};

class SplitTree {
private:
    // Forbid copy constructor and copy assignment operator.
    SplitTree(const SplitTree &);
    SplitTree &operator=(const SplitTree &);

    Node *root;

public:
    SplitTree();
    void set_root(AbstractState *single);
    Node *get_node(const state_var_t *buffer) const;
};
}

#endif
