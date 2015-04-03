#ifndef CEGAR_SPLIT_TREE_H
#define CEGAR_SPLIT_TREE_H

#include <cassert>
#include <utility>
#include <vector>

class State;

namespace cegar {
class Node {
    /*
      While right_child always corresponds to a (possibly split) abstract
      state, left_child may correspond to a helper node. Helper nodes
      are added to the hierarchy to allow for efficient lookup in case
      more than one atom is split off a state.
    */
    Node *left_child;
    Node *right_child;
    // Save the variable and value for which we split.
    int var;
    int value;
    // Estimated cost to goal node.
    int h;

    void split(int var, int value, Node *left_child, Node *right_child);

public:
    Node();
    ~Node() = default;

    Node(const Node &) = delete;
    Node &operator=(const Node &) = delete;

    /*
      Update the split tree for the new split. Additionally to the left and
      right child nodes add |values|-1 OR-nodes that all have the right child as
      their right child and the next OR-node as their left child.
    */
    std::pair<Node *, Node *> split(int var, const std::vector<int> &values);

    bool is_split() const {
        assert((!left_child && !right_child && var == -1 && value == -1) ||
               (left_child && right_child && var != -1 && value != -1));
        return left_child;
    }

    int get_var() const {
        assert(is_split());
        return var;
    }

    Node *get_child(int value) const;

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
