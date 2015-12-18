#include "split_tree.h"

#include "../task_proxy.h"

using namespace std;

namespace CEGAR {
Node::Node()
    : left_child(nullptr),
      right_child(nullptr),
      var(UNDEFINED),
      value(UNDEFINED),
      h(0) {
}

Node::~Node() {
    if (is_split()) {
        if (owns_right_child()) {
            delete right_child;
        }
        delete left_child;
    }
}

void Node::set_members(int var, int value, Node *left_child, Node *right_child) {
    this->var = var;
    this->value = value;
    this->left_child = left_child;
    this->right_child = right_child;
    assert(is_split());
}

pair<Node *, Node *> Node::split(int var, const vector<int> &values) {
    Node *helper = this;
    right_child = new Node();
    for (int value : values) {
        Node *new_helper = new Node();
        helper->set_members(var, value, new_helper, right_child);
        helper = new_helper;
    }
    assert(!helper->is_split());
    return make_pair(helper, right_child);
}

Node *Node::get_child(int value) const {
    assert(is_split());
    if (value == this->value)
        return right_child;
    return left_child;
}


SplitTree::SplitTree()
    : root(new Node()) {
}

SplitTree::SplitTree(SplitTree &&other)
    : root(move(other.root)) {
}

Node *SplitTree::get_node(const State &state) const {
    assert(root);
    Node *current = root.get();
    while (current->is_split()) {
        current = current->get_child(state[current->get_var()].get_value());
    }
    return current;
}
}
