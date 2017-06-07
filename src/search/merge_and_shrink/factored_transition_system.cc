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
    const bool compute_init_distances,
    const bool compute_goal_distances,
    const bool prune_unreachable_states,
    const bool prune_irrelevant_states,
    Verbosity verbosity,
    bool finalize_if_unsolvable)
    : labels(move(labels)),
      transition_systems(move(transition_systems)),
      mas_representations(move(mas_representations)),
      distances(move(distances)),
      compute_init_distances(compute_init_distances),
      compute_goal_distances(compute_goal_distances),
      unsolvable_index(-1),
      num_active_entries(this->transition_systems.size()) {
    for (size_t index = 0; index < this->transition_systems.size(); ++index) {
        if (compute_init_distances || compute_goal_distances) {
            this->distances[index]->compute_distances(
                compute_init_distances, compute_goal_distances, verbosity);
            /*
              While pruning is not part of the invariant of factors, we by
              default prune here because if we detect a factor as unsolvable,
              we can immediately stop, without computing distances for the
              remaining factors.
            */
            if (prune_unreachable_states || prune_irrelevant_states) {
                prune(
                    index,
                    prune_unreachable_states,
                    prune_irrelevant_states,
                    verbosity,
                    finalize_if_unsolvable);
            }
        }
        if (finalize_if_unsolvable && !this->transition_systems[index]->is_solvable()) {
            unsolvable_index = index;
            break;
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

bool FactoredTransitionSystem::apply_abstraction(
    int index,
    const StateEquivalenceRelation &state_equivalence_relation,
    Verbosity verbosity,
    bool finalize_if_unsolvable) {
    assert(is_active(index));

    int new_num_states = state_equivalence_relation.size();
    if (new_num_states == transition_systems[index]->get_size()) {
        if (verbosity >= Verbosity::VERBOSE) {
            cout << transition_systems[index]->tag()
                 << "not applying abstraction (same number of states)" << endl;
        }
        return false;
    }

    /* Compute the abstraction mapping based on the given state equivalence
       relation. */
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

    transition_systems[index]->apply_abstraction(
        state_equivalence_relation, abstraction_mapping, verbosity);
    if (compute_init_distances || compute_goal_distances) {
        distances[index]->apply_abstraction(
            state_equivalence_relation,
            verbosity,
            compute_init_distances,
            compute_goal_distances);
    }
    mas_representations[index]->apply_abstraction_to_lookup_table(
        abstraction_mapping);
    /*
      If this method is called from prune, then the transition system can
      become unsolvable.
    */
    if (finalize_if_unsolvable && !transition_systems[index]->is_solvable()) {
        unsolvable_index = index;
        return true;
    }

    /* If distances need to be recomputed, this already happened in the
       Distances object. */
    assert(is_component_valid(index));
    return true;
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
           && transition_systems[index]->are_transitions_sorted_unique()
           && transition_systems[index]->is_solvable();
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
    assert(is_component_valid(index1));
    assert(is_component_valid(index2));
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
    // Restore the invariant that distances are computed.
    if (compute_init_distances || compute_goal_distances) {
        distances[new_index]->compute_distances(
            compute_init_distances, compute_goal_distances, verbosity);
    }
    // TODO: can this ever trigger, given that both components are solvable?
    if (finalize_if_unsolvable && !new_ts.is_solvable()) {
        unsolvable_index = new_index;
    }
    --num_active_entries;
    assert(is_component_valid(new_index));
    return new_index;
}

bool FactoredTransitionSystem::prune(
    int index,
    bool prune_unreachable_states,
    bool prune_irrelevant_states,
    Verbosity verbosity,
    bool finalize_if_unsolvable) {
    assert(is_component_valid(index));
    assert(prune_unreachable_states || prune_irrelevant_states);
    const Distances &dist = *distances[index];
    int num_states = transition_systems[index]->get_size();
    StateEquivalenceRelation state_equivalence_relation;
    state_equivalence_relation.reserve(num_states);
    int unreachable_count = 0;
    int irrelevant_count = 0;
    int inactive_count = 0;
    for (int state = 0; state < num_states; ++state) {
        /* If pruning both unreachable and irrelevant states, a state which is
           counted for both statistics! */
        bool prune_state = false;
        if (prune_unreachable_states && dist.get_init_distance(state) == INF) {
            ++unreachable_count;
            prune_state = true;
        }
        if (prune_irrelevant_states && dist.get_goal_distance(state) == INF) {
            ++irrelevant_count;
            prune_state = true;
        }
        if (prune_state) {
            ++inactive_count;
        } else {
            StateEquivalenceClass state_equivalence_class;
            state_equivalence_class.push_front(state);
            state_equivalence_relation.push_back(state_equivalence_class);
        }
    }
    if (verbosity >= Verbosity::VERBOSE &&
        (unreachable_count || irrelevant_count)) {
        cout << transition_systems[index]->tag()
             << "unreachable: " << unreachable_count << " states, "
             << "irrelevant: " << irrelevant_count << " states ("
             << "total inactive: " << inactive_count << ")" << endl;
    }
    return apply_abstraction(index, state_equivalence_relation, verbosity, finalize_if_unsolvable);
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
