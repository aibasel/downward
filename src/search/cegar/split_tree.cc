#include "split_tree.h"

#include "abstract_state.h"

using namespace std;

namespace cegar {
Node::Node()
    : left_child(nullptr),
      right_child(nullptr),
      var(-1),
      value(-1),
      h(0) {
}

pair<Node *, Node *> Node::split(int var, const vector<int> &values) {
    Node *helper = this;
    right_child = new Node();
    for (int value : values) {
        helper->var = var;
        helper->value = value;
        helper->right_child = right_child;
        Node *new_helper = new Node();
        helper->left_child = new_helper;
        helper = new_helper;
    }
    assert(!helper->is_split());
    return make_pair(helper, right_child);
}

Node *Node::get_child(int value) const {
    if (value == this->value)
        return right_child;
    return left_child;
}


SplitTree::SplitTree()
    : root(new Node()) {
}

Node *SplitTree::get_node(const State &state) const {
    assert(root);
    Node *current = root;
    while (current->is_split()) {
        current = current->get_child(state[current->get_var()].get_value());
    }
    assert(!current->is_split());
    return current;
}
}
