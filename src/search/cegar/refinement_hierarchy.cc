#include "refinement_hierarchy.h"

#include "../task_proxy.h"

using namespace std;

namespace CEGAR {
Node::Node()
    : left_child(nullptr),
      right_child(nullptr),
      var(LEAF_NODE),
      value(LEAF_NODE),
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

pair<Node *, Node *> Node::split(int var, const vector<int> &values) {
    Node *helper = this;
    right_child = new Node();
    for (int value : values) {
        Node *new_helper = new Node();
        helper->var = var;
        helper->value = value;
        helper->left_child = new_helper;
        helper->right_child = right_child;
        assert(helper->is_split());
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


RefinementHierarchy::RefinementHierarchy()
    : root(new Node()) {
}

RefinementHierarchy::RefinementHierarchy(RefinementHierarchy &&other)
    : root(move(other.root)) {
}

Node *RefinementHierarchy::get_node(const State &state) const {
    assert(root);
    Node *current = root.get();
    while (current->is_split()) {
        current = current->get_child(state[current->get_var()].get_value());
    }
    return current;
}
}
