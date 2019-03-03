#include "refinement_hierarchy.h"

#include "../task_proxy.h"

using namespace std;

namespace cegar {
Node::Node(int state_id)
    : left_child(UNDEFINED),
      right_child(UNDEFINED),
      var(UNDEFINED),
      value(UNDEFINED),
      state_id(state_id) {
    assert(state_id != UNDEFINED);
}

NodeID Node::get_child(int value) const {
    assert(is_split());
    if (value == this->value)
        return right_child;
    return left_child;
}

ostream &operator<<(ostream &os, const Node &node) {
    return os << "<Node: var=" << node.var << " value=" << node.value
              << " state=" << node.state_id << " left=" << node.left_child
              << " right=" << node.right_child << ">";
}


RefinementHierarchy::RefinementHierarchy(const shared_ptr<AbstractTask> &task)
    : task(task) {
    nodes.emplace_back(0);
}

NodeID RefinementHierarchy::add_node(int state_id) {
    NodeID node_id = nodes.size();
    nodes.emplace_back(state_id);
    return node_id;
}

NodeID RefinementHierarchy::get_node_id(const State &state) const {
    NodeID id = 0;
    while (nodes[id].is_split()) {
        const Node &node = nodes[id];
        id = node.get_child(state[node.get_var()].get_value());
    }
    return id;
}

pair<NodeID, NodeID> RefinementHierarchy::split(
    NodeID node_id, int var, const vector<int> &values, int left_state_id, int right_state_id) {
    NodeID helper_id = node_id;
    NodeID right_child_id = add_node(right_state_id);
    for (int value : values) {
        NodeID new_helper_id = add_node(left_state_id);
        Node &helper = nodes[node_id];
        helper.var = var;
        helper.value = value;
        helper.left_child = new_helper_id;
        helper.right_child = right_child_id;
        helper.state_id = UNDEFINED;
        assert(helper.is_split());
        helper_id = new_helper_id;
    }
    return make_pair(helper_id, right_child_id);
}

int RefinementHierarchy::get_abstract_state_id(const State &state) const {
    TaskProxy subtask_proxy(*task);
    State subtask_state = subtask_proxy.convert_ancestor_state(state);
    return nodes[get_node_id(subtask_state)].get_state_id();
}
}
