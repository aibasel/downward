#include "factored_transition_system.h"

#include "distances.h"
#include "label_reduction.h"
#include "labels.h"
#include "merge_and_shrink_representation.h"
#include "shrink_strategy.h"
#include "transition_system.h"

#include "../utils/collections.h"
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
    Verbosity verbosity,
    bool finalize_if_unsolvable)
    : labels(move(labels)),
      transition_systems(move(transition_systems)),
      mas_representations(move(mas_representations)),
      distances(move(distances)),
      unsolvable_index(-1),
      num_active_entries(this->transition_systems.size()) {
    for (size_t i = 0; i < this->transition_systems.size(); ++i) {
        compute_distances_and_prune(i, verbosity);
        if (finalize_if_unsolvable && !this->transition_systems[i]->is_solvable()) {
            unsolvable_index = i;
            break;
        }
    }
}

FactoredTransitionSystem::FactoredTransitionSystem(FactoredTransitionSystem &&other)
    : labels(move(other.labels)),
      transition_systems(move(other.transition_systems)),
      mas_representations(move(other.mas_representations)),
      distances(move(other.distances)),
      unsolvable_index(move(other.unsolvable_index)),
      num_active_entries(move(other.num_active_entries)) {
    /*
      This is just a default move constructor. Unfortunately Visual
      Studio does not support "= default" for move construction or
      move assignment as of this writing.
    */
}

FactoredTransitionSystem::~FactoredTransitionSystem() {
}

void FactoredTransitionSystem::compute_distances_and_prune(
    int index, Verbosity verbosity) {
    /*
      This method does all that compute_distances does and
      additionally prunes all states that are unreachable (abstract g
      is infinite) or irrelevant (abstract h is infinite).
    */
    assert(is_active(index));
    discard_states(
        index,
        distances[index]->compute_distances(verbosity),
        verbosity);
    assert(is_component_valid(index));
}

void FactoredTransitionSystem::discard_states(
    int index,
    const vector<bool> &to_be_pruned_states,
    Verbosity verbosity) {
    assert(is_active(index));
    int num_states = transition_systems[index]->get_size();
    assert(static_cast<int>(to_be_pruned_states.size()) == num_states);
    StateEquivalenceRelation state_equivalence_relation;
    state_equivalence_relation.reserve(num_states);
    for (int state = 0; state < num_states; ++state) {
        if (!to_be_pruned_states[state]) {
            StateEquivalenceClass state_equivalence_class;
            state_equivalence_class.push_front(state);
            state_equivalence_relation.push_back(state_equivalence_class);
        }
    }
    apply_abstraction(index, state_equivalence_relation, verbosity);
}

bool FactoredTransitionSystem::apply_abstraction(
    int index,
    const StateEquivalenceRelation &state_equivalence_relation,
    Verbosity verbosity) {
    assert(is_active(index));

    vector<int> abstraction_mapping(
        transition_systems[index]->get_size(), PRUNED_STATE);
    for (size_t class_no = 0; class_no < state_equivalence_relation.size(); ++class_no) {
        const StateEquivalenceClass &state_equivalence_class =
            state_equivalence_relation[class_no];
        for (auto pos = state_equivalence_class.begin();
             pos != state_equivalence_class.end(); ++pos) {
            int state = *pos;
            assert(abstraction_mapping[state] == PRUNED_STATE);
            abstraction_mapping[state] = class_no;
        }
    }

    bool shrunk = transition_systems[index]->apply_abstraction(
        state_equivalence_relation, abstraction_mapping, verbosity);
    if (shrunk) {
        distances[index]->apply_abstraction(
            state_equivalence_relation, verbosity);
        mas_representations[index]->apply_abstraction_to_lookup_table(
            abstraction_mapping);
    }
    assert(is_component_valid(index));
    return shrunk;
}

void FactoredTransitionSystem::assert_index_valid(int index) const {
    assert(utils::in_bounds(index, transition_systems));
    assert(utils::in_bounds(index, mas_representations));
    assert(utils::in_bounds(index, distances));
    if (!(transition_systems[index] && mas_representations[index] && distances[index]) &&
        !(!transition_systems[index] && !mas_representations[index] && !distances[index])) {
        cerr << "Factor at index is in an inconsistent state!" << endl;
        utils::exit_with(utils::ExitCode::CRITICAL_ERROR);
    }
}

bool FactoredTransitionSystem::is_component_valid(int index) const {
    assert(is_active(index));
    return distances[index]->are_distances_computed()
           && transition_systems[index]->are_transitions_sorted_unique();
}

void FactoredTransitionSystem::apply_label_mapping(
    const vector<pair<int, vector<int>>> &label_mapping,
    int combinable_index) {
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
}

bool FactoredTransitionSystem::apply_label_reduction(
    const LabelReduction &label_reduction,
    const pair<int, int> &merge_indices,
    Verbosity verbosity) {
    return label_reduction.reduce(merge_indices, *this, verbosity);
}

bool FactoredTransitionSystem::shrink(
    int index,
    int target_size,
    const ShrinkStrategy &shrink_strategy,
    Verbosity verbosity) {
    const TransitionSystem &ts = *transition_systems[index];
    assert(ts.is_solvable());
    const Distances &dist = *distances[index];
    StateEquivalenceRelation equivalence_relation =
        shrink_strategy.compute_equivalence_relation(ts, dist, target_size);
    // TODO: We currently violate this; see issue250
    //assert(equivalence_relation.size() <= target_size);
    return apply_abstraction(index, equivalence_relation, verbosity);
}

int FactoredTransitionSystem::merge(
    int index1,
    int index2,
    Verbosity verbosity,
    bool finalize_if_unsolvable) {
    assert_index_valid(index1);
    assert_index_valid(index2);
    transition_systems.push_back(
        TransitionSystem::merge(
            *labels,
            *transition_systems[index1],
            *transition_systems[index2],
            verbosity));
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
    compute_distances_and_prune(new_index, verbosity);
    assert(is_component_valid(new_index));
    if (finalize_if_unsolvable && !new_ts.is_solvable()) {
        unsolvable_index = new_index;
    }
    --num_active_entries;
    return new_index;
}

pair<unique_ptr<MergeAndShrinkRepresentation>, unique_ptr<Distances>>
FactoredTransitionSystem::get_final_entry() {
    int final_index;
    if (unsolvable_index == -1) {
        /*
          If unsolvable_index == -1, we "regularly" finished the merge-and-
          shrink construction, i.e. we merged all transition systems and are
          left with one solvable transition system. This assumes that merges
          are always appended at the end.
        */
        for (size_t i = 0; i < transition_systems.size() - 1; ++i) {
            assert(!transition_systems[i]);
        }
        final_index = transition_systems.size() - 1;
        assert(transition_systems[final_index]->is_solvable());
        cout << "Final transition system size: "
             << transition_systems[final_index]->get_size() << endl;
    } else {
        // unsolvable_index points to an unsolvable transition system which
        // we use as return value.
        final_index = unsolvable_index;
        cout << "Abstract problem is unsolvable!" << endl;
    }

    return make_pair(move(mas_representations[final_index]),
                     move(distances[final_index]));
}

void FactoredTransitionSystem::statistics(int index) const {
    assert(is_active(index));
    const TransitionSystem &ts = *transition_systems[index];
    ts.statistics();
    const Distances &dist = *distances[index];
    dist.statistics();
}

void FactoredTransitionSystem::dump(int index) const {
    assert(transition_systems[index]);
    transition_systems[index]->dump_labels_and_transitions();
    mas_representations[index]->dump();
}

bool FactoredTransitionSystem::is_active(int index) const {
    assert_index_valid(index);
    return transition_systems[index] != nullptr;
}
}
