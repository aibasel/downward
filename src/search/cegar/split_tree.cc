#include "split_tree.h"

#include "abstract_state.h"

using namespace std;

namespace cegar_heuristic {

Node::Node(AbstractState *state)
    : abs_state(state),
      var(UNDEFINED),
      value(UNDEFINED),
      left_child(0),
      right_child(0) {
    if (state)
        state->set_node(this);
}

void Node::split(int var, int value, AbstractState *left, AbstractState *right) {
    this->var = var;
    this->value = value;
    left_child = new Node(left);
    right_child = new Node(right);
}

void Node::split(int var, const vector<int> &values, AbstractState *left, AbstractState *right) {
    Node *helper = this;
    right_child = new Node(right);
    for (int i = 0; i < values.size(); ++i) {
        helper->var = var;
        helper->value = values[i];
        helper->right_child = right_child;
        Node *new_helper = new Node();
        helper->left_child = new_helper;
        helper = new_helper;
    }
    assert(helper->var == UNDEFINED);
    helper->abs_state = left;
}

Node *Node::get_child(int value) const {
    if (value == this->value)
        return right_child;
    return left_child;
}

void SplitTree::set_root(AbstractState *single) {
    root = new Node(single);
}

Node *SplitTree::get_node(const State conc_state) const {
    assert(root);
    Node *current = root;
    while (current->is_split()) {
        current = current->get_child(conc_state[current->get_var()]);
    }
    assert(!current->is_split());
    return current;
}

}
