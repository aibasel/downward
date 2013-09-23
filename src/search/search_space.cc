#include "globals.h"
#include "search_space.h"
#include "state.h"
#include "operator.h"

#include <cassert>
#include <ext/hash_map>
#include "search_node_info.h"

using namespace std;
using namespace __gnu_cxx;


SearchNode::SearchNode(const StateID &state_id_,
                       SearchNodeInfo &info_, OperatorCost cost_type_)
    : state_id(state_id_), info(info_), cost_type(cost_type_) {
    assert(state_id.value >= 0);
}

State SearchNode::get_state() const {
    return g_state_registry->get_state(state_id);
}

bool SearchNode::is_open() const {
    return info.status == SearchNodeInfo::OPEN;
}

bool SearchNode::is_closed() const {
    return info.status == SearchNodeInfo::CLOSED;
}

bool SearchNode::is_dead_end() const {
    return info.status == SearchNodeInfo::DEAD_END;
}

bool SearchNode::is_new() const {
    return info.status == SearchNodeInfo::NEW;
}

int SearchNode::get_g() const {
    return info.g;
}

int SearchNode::get_real_g() const {
    return info.real_g;
}

int SearchNode::get_h() const {
    return info.h;
}

bool SearchNode::is_h_dirty() const {
    return info.h_is_dirty;
}

void SearchNode::set_h_dirty() {
    info.h_is_dirty = true;
}

void SearchNode::clear_h_dirty() {
    info.h_is_dirty = false;
}

void SearchNode::open_initial(int h) {
    assert(info.status == SearchNodeInfo::NEW);
    info.status = SearchNodeInfo::OPEN;
    info.g = 0;
    info.real_g = 0;
    info.h = h;
    info.parent_state_id = StateID::no_state;
    info.creating_operator = 0;
}

void SearchNode::open(int h, const SearchNode &parent_node,
                      const Operator *parent_op) {
    assert(info.status == SearchNodeInfo::NEW);
    info.status = SearchNodeInfo::OPEN;
    info.g = parent_node.info.g + get_adjusted_action_cost(*parent_op, cost_type);
    info.real_g = parent_node.info.real_g + parent_op->get_cost();
    info.h = h;
    info.parent_state_id = parent_node.get_state_id();
    info.creating_operator = parent_op;
}

void SearchNode::reopen(const SearchNode &parent_node,
                        const Operator *parent_op) {
    assert(info.status == SearchNodeInfo::OPEN ||
           info.status == SearchNodeInfo::CLOSED);

    // The latter possibility is for inconsistent heuristics, which
    // may require reopening closed nodes.
    info.status = SearchNodeInfo::OPEN;
    info.g = parent_node.info.g + get_adjusted_action_cost(*parent_op, cost_type);
    info.real_g = parent_node.info.real_g + parent_op->get_cost();
    info.parent_state_id = parent_node.get_state_id();
    info.creating_operator = parent_op;
}

// like reopen, except doesn't change status
void SearchNode::update_parent(const SearchNode &parent_node,
                               const Operator *parent_op) {
    assert(info.status == SearchNodeInfo::OPEN ||
           info.status == SearchNodeInfo::CLOSED);
    // The latter possibility is for inconsistent heuristics, which
    // may require reopening closed nodes.
    info.g = parent_node.info.g + get_adjusted_action_cost(*parent_op, cost_type);
    info.real_g = parent_node.info.real_g + parent_op->get_cost();
    info.parent_state_id = parent_node.get_state_id();
    info.creating_operator = parent_op;
}

void SearchNode::increase_h(int h) {
    assert(h >= info.h);
    info.h = h;
}

void SearchNode::close() {
    assert(info.status == SearchNodeInfo::OPEN);
    info.status = SearchNodeInfo::CLOSED;
}

void SearchNode::mark_as_dead_end() {
    info.status = SearchNodeInfo::DEAD_END;
}

void SearchNode::dump() const {
    cout << state_id.value << ": ";
    g_state_registry->get_state(state_id).dump_fdr();
    if (info.creating_operator) {
        cout << " created by " << info.creating_operator->get_name()
             << " from " << info.parent_state_id.value << endl;
    } else {
        cout << " no parent" << endl;
    }
}

SearchSpace::SearchSpace(OperatorCost cost_type_)
    : cost_type(cost_type_) {
}

SearchNode SearchSpace::get_node(const StateID &id) {
    return SearchNode(id, search_node_infos[id], cost_type);
}

void SearchSpace::trace_path(const State &goal_state,
                             vector<const Operator *> &path) const {
    StateID current_state_id = goal_state.get_id();
    assert(path.empty());
    for (;;) {
        const SearchNodeInfo &info = search_node_infos[current_state_id];
        const Operator *op = info.creating_operator;
        if (op == 0) {
            assert(info.parent_state_id.represents_no_state());
            break;
        }
        path.push_back(op);
        current_state_id = info.parent_state_id;
    }
    reverse(path.begin(), path.end());
}

void SearchSpace::dump() const {
    for (size_t i = 0; i < g_state_registry->size(); ++i) {
        StateID id(i);
        const SearchNodeInfo &node_info = search_node_infos[id];
        cout << "#" << i << ": ";
        g_state_registry->get_state(id).dump_fdr();
        if (node_info.creating_operator && !node_info.parent_state_id.represents_no_state()) {
            cout << " created by " << node_info.creating_operator->get_name()
                 << " from " << node_info.parent_state_id.value << endl;
        } else {
            cout << "has no parent" << endl;
        }
    }
}

void SearchSpace::statistics() const {
    cout << "Number of registered states: " << g_state_registry->size() << endl;
}
