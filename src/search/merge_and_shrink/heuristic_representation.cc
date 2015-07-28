#include "heuristic_representation.h"

#include "transition_system.h" // TODO: Try to get rid of this.

#include "../task_proxy.h"

#include <numeric>

using namespace std;


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
    cout << "leaf: applying abstraction to lookup table" << endl;

    int new_domain_size = 0;
    for (int &entry : lookup_table) {
        if (entry != TransitionSystem::PRUNED_STATE) {
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


HeuristicRepresentationMerge::HeuristicRepresentationMerge(
    unique_ptr<HeuristicRepresentation> child1,
    unique_ptr<HeuristicRepresentation> child2)
    : HeuristicRepresentation(child1->get_domain_size() *
                              child2->get_domain_size()),
      children {move(child1), move(child2)},
lookup_table(children[0]->get_domain_size(),
             vector<int>(children[1]->get_domain_size())) {
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
    cout << "merge: applying abstraction to lookup table" << endl;
    int new_domain_size = 0;
    for (vector<int> &row : lookup_table) {
        for (int &entry : row) {
            if (entry != TransitionSystem::PRUNED_STATE) {
                entry = abstraction_mapping[entry];
                new_domain_size = max(new_domain_size, entry + 1);
            }
        }
    }
    domain_size = new_domain_size;
}

int HeuristicRepresentationMerge::get_abstract_state(
    const State &state) const {
    int state1 = children[0]->get_abstract_state(state);
    int state2 = children[1]->get_abstract_state(state);
    if (state1 == TransitionSystem::PRUNED_STATE ||
        state2 == TransitionSystem::PRUNED_STATE)
        return TransitionSystem::PRUNED_STATE;
    return lookup_table[state1][state2];
}
