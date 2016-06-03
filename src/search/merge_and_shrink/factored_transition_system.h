#ifndef MERGE_AND_SHRINK_FACTORED_TRANSITION_SYSTEM_H
#define MERGE_AND_SHRINK_FACTORED_TRANSITION_SYSTEM_H

#include "types.h"

#include <memory>
#include <vector>

class State;
class TaskProxy;

namespace merge_and_shrink {
class Distances;
class HeuristicRepresentation;
class Labels;
class TransitionSystem;

class FactoredTransitionSystem {
    std::unique_ptr<Labels> labels;
    // Entries with nullptr have been merged.
    std::vector<std::unique_ptr<TransitionSystem>> transition_systems;
    std::vector<std::unique_ptr<HeuristicRepresentation>> heuristic_representations;
    std::vector<std::unique_ptr<Distances>> distances;
    int final_index;
    bool solvable;
    // TODO: add something like "current_index"? for shrink classes e.g.

    void compute_distances_and_prune(int index);
    void discard_states(int index, const std::vector<bool> &to_be_pruned_states);

    bool is_index_valid(int index) const;
    bool is_component_valid(int index) const;
    bool is_finalized() const {
        return final_index != -1;
    }
public:
    FactoredTransitionSystem(
        std::unique_ptr<Labels> labels,
        std::vector<std::unique_ptr<TransitionSystem>> &&transition_systems,
        std::vector<std::unique_ptr<HeuristicRepresentation>> &&heuristic_representations,
        std::vector<std::unique_ptr<Distances>> &&distances);
    FactoredTransitionSystem(FactoredTransitionSystem &&other);
    ~FactoredTransitionSystem();

    // No copying or assignment.
    FactoredTransitionSystem(const FactoredTransitionSystem &) = delete;
    FactoredTransitionSystem &operator=(
        const FactoredTransitionSystem &) = delete;

    const TransitionSystem &get_ts(int index) const {
        return *transition_systems[index];
    }
    const Distances &get_dist(int index) const {
        return *distances[index];
    }

    // Methods for the merge-and-shrink main loop
    void apply_label_reduction(
        const std::vector<std::pair<int, std::vector<int>>> &label_mapping,
        int combinable_index);
    bool apply_abstraction(
        int index,
        const StateEquivalenceRelation &state_equivalence_relation);
    int merge(int index1, int index2);
    void finalize(int index = -1);
    bool is_solvable() const {
        return solvable;
    }
    int get_cost(const State &state) const;
    void statistics(int index) const;
    void dump(int index) const;

    // the following methods are used by merge strategies
    // TODO: maybe we need a more convient way of iterating over all
    // transition systems or even over all combined transition systems/
    // heuristic representations/distances tuples?
    int get_size() const {
        return transition_systems.size();
    }
    bool is_active(int index) const {
        return is_index_valid(index);
    }
    // TODO: remove the following method and let DFP use get_labels?
    int get_num_labels() const; // used by merge_dfp
    const Labels &get_labels() const { // used by label_reduction
        return *labels;
    }
};
}

#endif
