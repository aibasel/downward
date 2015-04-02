#include "split_tree.h"

#include "abstract_state.h"

using namespace std;

namespace cegar {
Node::Node(AbstractState *state)
    : abs_state(state),
      left_child(0),
      right_child(0),
      var(UNDEFINED),
      value(UNDEFINED),
      h(0) {
    if (state)
        state->set_node(this);
}

void Node::split(int var, const vector<int> &values, AbstractState *left, AbstractState *right) {
    Node *helper = this;
    right_child = new Node(right);
    for (size_t i = 0; i < values.size(); ++i) {
        helper->var = var;
        helper->value = values[i];
        helper->right_child = right_child;
        Node *new_helper = new Node();
        helper->left_child = new_helper;
        helper = new_helper;
    }
    assert(helper->var == UNDEFINED);
    helper->abs_state = left;
    left->set_node(helper);
}

Node *Node::get_child(int value) const {
    if (value == this->value)
        return right_child;
    return left_child;
}

Node *Node::get_left_child() const {
    return left_child;
}

Node *Node::get_right_child() const {
    return right_child;
}


SplitTree::SplitTree()
    : root(0) {
}

void SplitTree::set_root(AbstractState *single) {
    root = new Node(single);
}

Node *SplitTree::get_node(const State &state) const {
    assert(root);
    Node *current = root;
    while (current->is_split()) {
        current = current->get_child(state[current->get_var()].get_value());
    }
    assert(!current->is_split());
    assert(current->get_abs_state());
    return current;
}
}
