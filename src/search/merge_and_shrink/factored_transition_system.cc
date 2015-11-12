#include "factored_transition_system.h"

#include "distances.h"
#include "labels.h"
#include "heuristic_representation.h"
#include "transition_system.h"

#include "../timer.h"

#include <cassert>

using namespace std;


FactoredTransitionSystem::FactoredTransitionSystem(
    shared_ptr<Labels> labels,
    vector<TransitionSystem *> &&transition_systems,
    vector<unique_ptr<HeuristicRepresentation>> &&heuristic_representations,
    vector<unique_ptr<Distances>> &&distances)
    : labels(labels),
      transition_systems(move(transition_systems)),
      heuristic_representations(move(heuristic_representations)),
      distances(move(distances)),
      final_index(-1),
      solvable(true) {
    for (size_t i = 0; i < this->transition_systems.size(); ++i) {
        compute_distances_and_prune(i);
        if (!this->transition_systems[i]->is_solvable()) {
            solvable = false;
            finalize(i);
            break;
        }
    }
}

FactoredTransitionSystem::FactoredTransitionSystem(FactoredTransitionSystem &&other)
    : labels(move(other.labels)),
      transition_systems(move(other.transition_systems)),
      heuristic_representations(move(other.heuristic_representations)),
      distances(move(other.distances)),
      final_index(move(other.final_index)),
      solvable(move(other.solvable)) {
    /*
      This is just a default move constructor. Unfortunately Visual
      Studio does not support "= default" for move construction or
      move assignment as of this writing.
    */
}

FactoredTransitionSystem::~FactoredTransitionSystem() {
}

void FactoredTransitionSystem::discard_states(int index,
                                              const vector<bool> &to_be_pruned_states) {
    assert(is_index_valid(index));
    int num_states = transition_systems[index]->get_size();
    assert(static_cast<int>(to_be_pruned_states.size()) == num_states);
    vector<forward_list<int>> equivalence_relation;
    equivalence_relation.reserve(num_states);
    for (int state = 0; state < num_states; ++state) {
        if (!to_be_pruned_states[state]) {
            forward_list<int> group;
            group.push_front(state);
            equivalence_relation.push_back(group);
        }
    }
    apply_abstraction(index, equivalence_relation);
}

bool FactoredTransitionSystem::is_index_valid(int index) const {
    return transition_systems[index] && heuristic_representations[index]
           && distances[index];
}

bool FactoredTransitionSystem::is_component_valid(int index) const {
    assert(is_index_valid(index));
    return distances[index]->are_distances_computed()
           && transition_systems[index]->are_transitions_sorted_unique();
}


void FactoredTransitionSystem::compute_distances_and_prune(int index) {
    /*
      This method does all that compute_distances does and
      additionally prunes all states that are unreachable (abstract g
      is infinite) or irrelevant (abstract h is infinite).
    */
    assert(is_index_valid(index));
    discard_states(index, distances[index]->compute_distances());
    assert(is_component_valid(index));
}

void FactoredTransitionSystem::apply_label_reduction(
    const vector<pair<int, vector<int>>> &label_mapping,
    int combinable_index) {
    for (size_t i = 0; i < transition_systems.size(); ++i) {
        if (transition_systems[i]) {
            transition_systems[i]->apply_label_reduction(
                label_mapping, static_cast<int>(i) != combinable_index);
        }
    }
}

bool FactoredTransitionSystem::apply_abstraction(
    int index, const vector<forward_list<int>> &collapsed_groups) {
    assert(is_index_valid(index));

    vector<int> abstraction_mapping(transition_systems[index]->get_size(), PRUNED_STATE);
    for (size_t group_no = 0; group_no < collapsed_groups.size(); ++group_no) {
        const auto &group = collapsed_groups[group_no];
        for (auto pos = group.begin(); pos != group.end(); ++pos) {
            AbstractStateRef state = *pos;
            assert(abstraction_mapping[state] == PRUNED_STATE);
            abstraction_mapping[state] = group_no;
        }
    }

    bool shrunk = transition_systems[index]->apply_abstraction(collapsed_groups, abstraction_mapping);
    if (!distances[index]->apply_abstraction(collapsed_groups))
        cout << transition_systems[index]->tag() << "simplification was not f-preserving!" << endl;
    if (shrunk) {
        heuristic_representations[index]->apply_abstraction_to_lookup_table(
            abstraction_mapping);
    }
    assert(is_component_valid(index));
    return shrunk;
}

int FactoredTransitionSystem::merge(const TaskProxy &task_proxy, int index1, int index2) {
    assert(is_index_valid(index1));
    assert(is_index_valid(index2));
    TransitionSystem *ts1 = transition_systems[index1];
    TransitionSystem *ts2 = transition_systems[index2];
    TransitionSystem *new_transition_system = new TransitionSystem(
        task_proxy, labels, ts1, ts2);
    transition_systems.push_back(new_transition_system);
    delete ts1;
    delete ts2;
    distances[index1] = nullptr;
    distances[index2] = nullptr;
    transition_systems[index1] = nullptr;
    transition_systems[index2] = nullptr;
    heuristic_representations.push_back(make_unique_ptr<HeuristicRepresentationMerge>(
                                            move(heuristic_representations[index1]),
                                            move(heuristic_representations[index2])));
    distances.push_back(make_unique_ptr<Distances>(*new_transition_system));
    int new_index = transition_systems.size() - 1;
    compute_distances_and_prune(new_index);
    assert(is_component_valid(new_index));
    if (!new_transition_system->is_solvable()) {
        solvable = false;
        finalize(new_index);
    }
    return new_index;
}


void FactoredTransitionSystem::finalize(int index) {
    if (index == -1) {
        /*
          This is the case if we "regularly" finished the merge-and-shrink
          construction, i.e. we merged all transition systems and are left
          with one. This assumes that merges are always appended at the end.
        */
        assert(solvable);
        final_index = transition_systems.size() - 1;
        for (size_t i = 0; i < transition_systems.size() - 1; ++i) {
            assert(!transition_systems[i]);
            assert(!distances[i]);
        }
        delete transition_systems[final_index];
        transition_systems[final_index] = nullptr;
    } else {
        /*
          If an index is given, this means that the specific transition system
          is unsolvable. We throw away all transition systems and all other
          distances.
        */
        assert(!solvable);
        final_index = index;
        for (size_t i = 0; i < transition_systems.size(); ++i) {
            delete transition_systems[i];
            transition_systems[i] = nullptr;
            if (static_cast<int>(i) != index) {
                distances[i] = nullptr;
            }
        }
    }
    transition_systems.clear();
}

int FactoredTransitionSystem::get_cost(const State &state) const {
    assert(is_finalized());
    assert(distances[final_index]->are_distances_computed());
    int abs_state = heuristic_representations[final_index]->get_abstract_state(state);

    if (abs_state == PRUNED_STATE)
        return -1;
    int cost = distances[final_index]->get_goal_distance(abs_state);
    assert(cost != INF);
    return cost;
}

void FactoredTransitionSystem::statistics(int index,
                                          const Timer &timer) const {
    assert(is_index_valid(index));
    TransitionSystem *ts = transition_systems[index];
    ts->statistics();
    // TODO: Turn the following block into Distances::statistics()?
    cout << ts->tag();
    if (!distances[index]->are_distances_computed()) {
        cout << "distances not computed";
    } else if (is_solvable()) {
        cout << "init h=" << distances[index]->get_goal_distance(ts->get_init_state())
             << ", max f=" << distances[index]->get_max_f()
             << ", max g=" << distances[index]->get_max_g()
             << ", max h=" << distances[index]->get_max_h();
    } else {
        cout << "transition system is unsolvable";
    }
    cout << " [t=" << timer << "]" << endl;
}

void FactoredTransitionSystem::dump(int index) const {
    assert(transition_systems[index]);
    transition_systems[index]->dump_labels_and_transitions();
    heuristic_representations[index]->dump();
}

int FactoredTransitionSystem::get_num_labels() const {
    return labels->get_size();
}
