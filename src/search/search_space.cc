#include "search_space.h"
#include "state.h"
#include "operator.h"

#include <cassert>
#include <ext/hash_map>
#include "state_proxy.h"
#include "search_node_info.h"

using namespace std;
using namespace __gnu_cxx;




SearchNode::SearchNode(state_var_t *state_buffer_, SearchNodeInfo &info_)
    : state_buffer(state_buffer_), info(info_) {
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

int SearchNode::get_h() const {
    return info.h;
}

const state_var_t *SearchNode::get_parent_buffer() const {
    return info.parent_state;
}

void SearchNode::open_initial(int h) {
    assert(info.status == SearchNodeInfo::NEW);
    info.status = SearchNodeInfo::OPEN;
    info.g = 0;
    info.h = h;
    info.parent_state = 0;
    info.creating_operator = 0;
}

void SearchNode::open(int h, const SearchNode &parent_node,
                      const Operator *parent_op) {
    assert(info.status == SearchNodeInfo::NEW);
    info.status = SearchNodeInfo::OPEN;
    info.g = parent_node.info.g + parent_op->get_cost();
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
    info.g = parent_node.info.g + parent_op->get_cost();
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
    info.g = parent_node.info.g + parent_op->get_cost();
    info.parent_state = parent_node.state_buffer;
    info.creating_operator = parent_op;
}

void SearchNode::close() {
    assert(info.status == SearchNodeInfo::OPEN);
    info.status = SearchNodeInfo::CLOSED;
}

void SearchNode::mark_as_dead_end() {
    assert(info.status == SearchNodeInfo::NEW);
    info.status = SearchNodeInfo::DEAD_END;
}

void SearchNode::dump() {
    cout << state_buffer << ": ";
    State(state_buffer).dump();
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


SearchSpace::SearchSpace() {
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
    return SearchNode(iter->first.state_data, iter->second);
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
        State(iter->first.state_data).dump();
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
    cout << "search space hash size: " << nodes->size() << endl;
    cout << "search space hash bucket count: " << nodes->bucket_count() << endl;
}
