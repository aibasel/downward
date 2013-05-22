#include "search_space.h"
#include "state.h"
#include "operator.h"

#include <cassert>
#include <ext/hash_map>
#include "state_proxy.h"
#include "search_node_info.h"

using namespace std;
using namespace __gnu_cxx;




SearchNode::SearchNode(state_var_t *state_buffer_, SearchNodeInfo &info_, OperatorCost cost_type_)
    : state_buffer(state_buffer_), info(info_), cost_type(cost_type_) {
}

State SearchNode::get_state() const {
    return State(state_buffer);
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

const state_var_t *SearchNode::get_parent_buffer() const {
    return info.parent_state;
}

void SearchNode::open_initial(int h) {
    assert(info.status == SearchNodeInfo::NEW);
    info.status = SearchNodeInfo::OPEN;
    info.g = 0;
    info.real_g = 0;
    info.h = h;
    info.parent_state = 0;
    info.creating_operator = 0;
}

void SearchNode::open(int h, const SearchNode &parent_node,
                      const Operator *parent_op) {
    assert(info.status == SearchNodeInfo::NEW);
    info.status = SearchNodeInfo::OPEN;
    info.g = parent_node.info.g + get_adjusted_action_cost(*parent_op, cost_type);
    info.real_g = parent_node.info.real_g + parent_op->get_cost();
    info.h = h;
    info.parent_state = parent_node.state_buffer;
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
    info.parent_state = parent_node.state_buffer;
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
    info.parent_state = parent_node.state_buffer;
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

void SearchNode::dump() {
    cout << state_buffer << ": ";
    State(state_buffer).dump_fdr();
    cout << " created by " << info.creating_operator->get_name()
         << " from " << info.parent_state << endl;
}

class SearchSpace::HashTable
    : public __gnu_cxx::hash_map<StateProxy, SearchNodeInfo> {
    // This is more like a typedef really, but we need a proper class
    // so that we can hide the information in the header file by using
    // a forward declaration. This is also the reason why the hash
    // table is allocated dynamically in the constructor.
};


SearchSpace::SearchSpace(OperatorCost cost_type_)
    : cost_type(cost_type_) {
    nodes = new HashTable;
}

SearchSpace::~SearchSpace() {
    delete nodes;
}

int SearchSpace::size() const {
    return nodes->size();
}

SearchNode SearchSpace::get_node(const State &state) {
    static SearchNodeInfo default_info;
    pair<HashTable::iterator, bool> result = nodes->insert(
        make_pair(StateProxy(&state), default_info));
    if (result.second) {
        // This is a new entry: Must give the state permanent lifetime.
        result.first->first.make_permanent();
    }
    HashTable::iterator iter = result.first;
    return SearchNode(iter->first.state_data, iter->second, cost_type);
}

void SearchSpace::trace_path(const State &goal_state,
                             vector<const Operator *> &path) const {
    StateProxy current_state(&goal_state);
    assert(path.empty());
    for (;;) {
        HashTable::const_iterator iter = nodes->find(current_state);
        assert(iter != nodes->end());
        const SearchNodeInfo &info = iter->second;
        const Operator *op = info.creating_operator;
        if (op == 0)
            break;
        path.push_back(op);
        current_state = StateProxy(const_cast<state_var_t *>(info.parent_state));
    }
    reverse(path.begin(), path.end());
}

void SearchSpace::dump() {
    int i = 0;
    for (HashTable::iterator iter = nodes->begin(); iter != nodes->end(); iter++) {
        cout << "#" << i++ << " (" << iter->first.state_data << "): ";
        State(iter->first.state_data).dump_fdr();
        if (iter->second.creating_operator &&
            iter->second.parent_state) {
            cout << " created by " << iter->second.creating_operator->get_name()
                 << " from " << iter->second.parent_state << endl;
        } else {
            cout << "has no parent" << endl;
        }
    }
}

void SearchSpace::statistics() const {
    cout << "Search space hash size: " << nodes->size() << endl;
    cout << "Search space hash bucket count: " << nodes->bucket_count() << endl;
}
