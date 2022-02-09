#include "factored_transition_system.h"

#include "distances.h"
#include "labels.h"
#include "merge_and_shrink_representation.h"
#include "transition_system.h"
#include "utils.h"

#include "../utils/collections.h"
#include "../utils/logging.h"
#include "../utils/memory.h"
#include "../utils/system.h"

#include <cassert>

using namespace std;

namespace merge_and_shrink {
FTSConstIterator::FTSConstIterator(
    const FactoredTransitionSystem &fts,
    bool end)
    : fts(fts), current_index((end ? fts.get_size() : 0)) {
    next_valid_index();
}

void FTSConstIterator::next_valid_index() {
    while (current_index < fts.get_size()
           && !fts.is_active(current_index)) {
        ++current_index;
    }
}

void FTSConstIterator::operator++() {
    ++current_index;
    next_valid_index();
}


FactoredTransitionSystem::FactoredTransitionSystem(
    unique_ptr<Labels> labels,
    vector<unique_ptr<TransitionSystem>> &&transition_systems,
    vector<unique_ptr<MergeAndShrinkRepresentation>> &&mas_representations,
    vector<unique_ptr<Distances>> &&distances,
    const bool compute_init_distances,
    const bool compute_goal_distances,
    utils::LogProxy &log)
    : labels(move(labels)),
      transition_systems(move(transition_systems)),
      mas_representations(move(mas_representations)),
      distances(move(distances)),
      compute_init_distances(compute_init_distances),
      compute_goal_distances(compute_goal_distances),
      num_active_entries(this->transition_systems.size()) {
    for (size_t index = 0; index < this->transition_systems.size(); ++index) {
        if (compute_init_distances || compute_goal_distances) {
            this->distances[index]->compute_distances(
                compute_init_distances, compute_goal_distances, log);
        }
        assert(is_component_valid(index));
    }
}

FactoredTransitionSystem::FactoredTransitionSystem(FactoredTransitionSystem &&other)
    : labels(move(other.labels)),
      transition_systems(move(other.transition_systems)),
      mas_representations(move(other.mas_representations)),
      distances(move(other.distances)),
      compute_init_distances(move(other.compute_init_distances)),
      compute_goal_distances(move(other.compute_goal_distances)),
      num_active_entries(move(other.num_active_entries)) {
    /*
      This is just a default move constructor. Unfortunately Visual
      Studio does not support "= default" for move construction or
      move assignment as of this writing.
    */
}

FactoredTransitionSystem::~FactoredTransitionSystem() {
}

void FactoredTransitionSystem::assert_index_valid(int index) const {
    assert(utils::in_bounds(index, transition_systems));
    assert(utils::in_bounds(index, mas_representations));
    assert(utils::in_bounds(index, distances));
    if (!(transition_systems[index] && mas_representations[index] && distances[index]) &&
        !(!transition_systems[index] && !mas_representations[index] && !distances[index])) {
        cerr << "Factor at index is in an inconsistent state!" << endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
}

bool FactoredTransitionSystem::is_component_valid(int index) const {
    assert(is_active(index));
    if (compute_init_distances && !distances[index]->are_init_distances_computed()) {
        return false;
    }
    if (compute_goal_distances && !distances[index]->are_goal_distances_computed()) {
        return false;
    }
    return transition_systems[index]->are_transitions_sorted_unique() &&
           transition_systems[index]->in_sync_with_label_equivalence_relation();
}

void FactoredTransitionSystem::assert_all_components_valid() const {
    for (size_t index = 0; index < transition_systems.size(); ++index) {
        if (transition_systems[index]) {
            assert(is_component_valid(index));
        }
    }
}

void FactoredTransitionSystem::apply_label_mapping(
    const vector<pair<int, vector<int>>> &label_mapping,
    int combinable_index) {
    assert_all_components_valid();
    for (const auto &new_label_old_labels : label_mapping) {
        assert(new_label_old_labels.first == labels->get_size());
        labels->reduce_labels(new_label_old_labels.second);
    }
    for (size_t i = 0; i < transition_systems.size(); ++i) {
        if (transition_systems[i]) {
            transition_systems[i]->apply_label_reduction(
                label_mapping, static_cast<int>(i) != combinable_index);
        }
    }
    assert_all_components_valid();
}

bool FactoredTransitionSystem::apply_abstraction(
    int index,
    const StateEquivalenceRelation &state_equivalence_relation,
    utils::LogProxy &log) {
    assert(is_component_valid(index));

    int new_num_states = state_equivalence_relation.size();
    if (new_num_states == transition_systems[index]->get_size()) {
        return false;
    }

    vector<int> abstraction_mapping = compute_abstraction_mapping(
        transition_systems[index]->get_size(), state_equivalence_relation);

    transition_systems[index]->apply_abstraction(
        state_equivalence_relation, abstraction_mapping, log);
    if (compute_init_distances || compute_goal_distances) {
        distances[index]->apply_abstraction(
            state_equivalence_relation,
            compute_init_distances,
            compute_goal_distances,
            log);
    }
    mas_representations[index]->apply_abstraction_to_lookup_table(
        abstraction_mapping);

    /* If distances need to be recomputed, this already happened in the
       Distances object. */
    assert(is_component_valid(index));
    return true;
}

int FactoredTransitionSystem::merge(
    int index1,
    int index2,
    utils::LogProxy &log) {
    assert(is_component_valid(index1));
    assert(is_component_valid(index2));
    transition_systems.push_back(
        TransitionSystem::merge(
            *labels,
            *transition_systems[index1],
            *transition_systems[index2],
            log));
    distances[index1] = nullptr;
    distances[index2] = nullptr;
    transition_systems[index1] = nullptr;
    transition_systems[index2] = nullptr;
    mas_representations.push_back(
        utils::make_unique_ptr<MergeAndShrinkRepresentationMerge>(
            move(mas_representations[index1]),
            move(mas_representations[index2])));
    mas_representations[index1] = nullptr;
    mas_representations[index2] = nullptr;
    const TransitionSystem &new_ts = *transition_systems.back();
    distances.push_back(utils::make_unique_ptr<Distances>(new_ts));
    int new_index = transition_systems.size() - 1;
    // Restore the invariant that distances are computed.
    if (compute_init_distances || compute_goal_distances) {
        distances[new_index]->compute_distances(
            compute_init_distances, compute_goal_distances, log);
    }
    --num_active_entries;
    assert(is_component_valid(new_index));
    return new_index;
}

pair<unique_ptr<MergeAndShrinkRepresentation>, unique_ptr<Distances>>
FactoredTransitionSystem::extract_factor(int index) {
    assert(is_component_valid(index));
    return make_pair(move(mas_representations[index]),
                     move(distances[index]));
}

void FactoredTransitionSystem::statistics(int index, utils::LogProxy &log) const {
    assert(is_component_valid(index));
    const TransitionSystem &ts = *transition_systems[index];
    ts.statistics(log);
    const Distances &dist = *distances[index];
    dist.statistics(log);
}

void FactoredTransitionSystem::dump(int index, utils::LogProxy &log) const {
    assert_index_valid(index);
    transition_systems[index]->dump_labels_and_transitions(log);
    mas_representations[index]->dump(log);
}

void FactoredTransitionSystem::dump(utils::LogProxy &log) const {
    for (int index : *this) {
        dump(index, log);
    }
}

bool FactoredTransitionSystem::is_factor_solvable(int index) const {
    assert(is_component_valid(index));
    return transition_systems[index]->is_solvable(*distances[index]);
}

bool FactoredTransitionSystem::is_factor_trivial(int index) const {
    assert(is_component_valid(index));
    if (!mas_representations[index]->is_total()) {
        return false;
    }
    const TransitionSystem &ts = *transition_systems[index];
    for (int state = 0; state < ts.get_size(); ++state) {
        if (!ts.is_goal_state(state)) {
            return false;
        }
    }
    return true;
}

bool FactoredTransitionSystem::is_active(int index) const {
    assert_index_valid(index);
    return transition_systems[index] != nullptr;
}
}
