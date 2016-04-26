#include "heuristic_representation.h"

#include "types.h"

#include "../task_proxy.h"

#include <algorithm>
#include <iostream>
#include <numeric>

using namespace std;

namespace merge_and_shrink {
HeuristicRepresentation::HeuristicRepresentation(int domain_size)
    : domain_size(domain_size) {
}

HeuristicRepresentation::~HeuristicRepresentation() {
}

int HeuristicRepresentation::get_domain_size() const {
    return domain_size;
}


HeuristicRepresentationLeaf::HeuristicRepresentationLeaf(
    int var_id, int domain_size)
    : HeuristicRepresentation(domain_size),
      var_id(var_id),
      lookup_table(domain_size) {
    iota(lookup_table.begin(), lookup_table.end(), 0);
}

void HeuristicRepresentationLeaf::apply_abstraction_to_lookup_table(
    const vector<int> &abstraction_mapping) {
    int new_domain_size = 0;
    for (int &entry : lookup_table) {
        if (entry != PRUNED_STATE) {
            entry = abstraction_mapping[entry];
            new_domain_size = max(new_domain_size, entry + 1);
        }
    }
    domain_size = new_domain_size;
}

int HeuristicRepresentationLeaf::get_abstract_state(const State &state) const {
    int value = state[var_id].get_value();
    return lookup_table[value];
}

void HeuristicRepresentationLeaf::dump() const {
    for (const auto &value : lookup_table) {
        cout << value << ", ";
    }
    cout << endl;
}


HeuristicRepresentationMerge::HeuristicRepresentationMerge(
    unique_ptr<HeuristicRepresentation> left_child_,
    unique_ptr<HeuristicRepresentation> right_child_)
    : HeuristicRepresentation(left_child_->get_domain_size() *
                              right_child_->get_domain_size()),
      left_child(move(left_child_)),
      right_child(move(right_child_)),
      lookup_table(left_child->get_domain_size(),
                   vector<int>(right_child->get_domain_size())) {
    int counter = 0;
    for (vector<int> &row : lookup_table) {
        for (int &entry : row) {
            entry = counter;
            ++counter;
        }
    }
}

void HeuristicRepresentationMerge::apply_abstraction_to_lookup_table(
    const vector<int> &abstraction_mapping) {
    int new_domain_size = 0;
    for (vector<int> &row : lookup_table) {
        for (int &entry : row) {
            if (entry != PRUNED_STATE) {
                entry = abstraction_mapping[entry];
                new_domain_size = max(new_domain_size, entry + 1);
            }
        }
    }
    domain_size = new_domain_size;
}

int HeuristicRepresentationMerge::get_abstract_state(
    const State &state) const {
    int state1 = left_child->get_abstract_state(state);
    int state2 = right_child->get_abstract_state(state);
    if (state1 == PRUNED_STATE ||
        state2 == PRUNED_STATE)
        return PRUNED_STATE;
    return lookup_table[state1][state2];
}

void HeuristicRepresentationMerge::dump() const {
    for (const auto &row : lookup_table) {
        for (const auto &value : row) {
            cout << value << ", ";
        }
        cout << endl;
    }
    cout << "dump left child:" << endl;
    left_child->dump();
    cout << "dump right child:" << endl;
    right_child->dump();
}
}
