#ifndef CEGAR_SPLIT_TREE_H
#define CEGAR_SPLIT_TREE_H

#include <cassert>
#include <memory>
#include <utility>
#include <vector>

class State;

namespace CEGAR {
class Node {
    static const int UNDEFINED = -2;
    /*
      While right_child is always the node of a (possibly split) abstract
      state, left_child may be a helper node. We add helper nodes to the
      hierarchy to allow for efficient lookup in case more than one fact is
      split off a state.
    */
    Node *left_child;
    Node *right_child;

    // Save the variable and value for which the corresponding state was split.
    int var;
    int value;

    // Estimated cost to nearest goal state from this node's state.
    int h;

    void set_members(int var, int value, Node *left_child, Node *right_child);

public:
    Node();
    ~Node();

    Node(const Node &) = delete;
    Node &operator=(const Node &) = delete;

    /*
      Update the split tree for the new split. Additionally to the left and
      right child nodes add |values|-1 helper nodes that all have the right
      child as their right child and the next helper node as their left child.
    */
    std::pair<Node *, Node *> split(int var, const std::vector<int> &values);

    bool is_split() const {
        assert((!left_child && !right_child &&
                var == UNDEFINED && value == UNDEFINED) ||
               (left_child && right_child &&
                var != UNDEFINED && value != UNDEFINED));
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

    void set_h_value(int new_h) {
        assert(new_h >= h);
        h = new_h;
    }

    int get_h_value() const {
        return h;
    }
};


class SplitTree {
    std::unique_ptr<Node> root;

public:
    SplitTree();
    ~SplitTree() = default;

    SplitTree(SplitTree &&) = default;

    Node *get_node(const State &state) const;
    Node *get_root() const {return root.get(); }
};
}

#endif
