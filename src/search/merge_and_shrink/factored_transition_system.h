#ifndef MERGE_AND_SHRINK_FACTORED_TRANSITION_SYSTEM_H
#define MERGE_AND_SHRINK_FACTORED_TRANSITION_SYSTEM_H

#include "types.h"

#include <memory>
#include <vector>

class State;
class TaskProxy;

namespace merge_and_shrink {
class Distances;
class FactoredTransitionSystem;
class HeuristicRepresentation;
class Labels;
class TransitionSystem;

class FTSConstIterator {
    /*
      This class allows users to easily iterate over the active indices of
      a factored transition system.
    */
    const FactoredTransitionSystem &fts;
    // current_index is the actual iterator
    int current_index;

    void next_valid_index();
public:
    FTSConstIterator(const FactoredTransitionSystem &fts, bool end);
    void operator++();

    int operator*() const {
        return current_index;
    }

    bool operator==(const FTSConstIterator &rhs) const {
        return current_index == rhs.current_index;
    }

    bool operator!=(const FTSConstIterator &rhs) const {
        return current_index != rhs.current_index;
    }
};

class FactoredTransitionSystem {
    std::unique_ptr<Labels> labels;
    // Entries with nullptr have been merged.
    std::vector<std::unique_ptr<TransitionSystem>> transition_systems;
    std::vector<std::unique_ptr<HeuristicRepresentation>> heuristic_representations;
    std::vector<std::unique_ptr<Distances>> distances;
    int final_index;
    bool solvable;
    // TODO: add something like "current_index"? for shrink classes e.g.

    void compute_distances_and_prune(
        int index,
        Verbosity verbosity);
    void discard_states(
        int index,
        const std::vector<bool> &to_be_pruned_states,
        Verbosity verbosity);

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
        std::vector<std::unique_ptr<Distances>> &&distances,
        Verbosity verbosity);
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

    // Methods for MergeAndShrinkHeuristic
    void apply_label_reduction(
        const std::vector<std::pair<int, std::vector<int>>> &label_mapping,
        int combinable_index);
    bool apply_abstraction(
        int index,
        const StateEquivalenceRelation &state_equivalence_relation,
        Verbosity verbosity);
    int merge(int index1, int index2, Verbosity verbosity);
    void finalize(int index = -1);

    bool is_solvable() const {
        return solvable;
    }

    int get_cost(const State &state) const;
    void statistics(int index) const;
    void dump(int index) const;

    // Used by LabelReduction and MergeScoringFunctionDFP
    const Labels &get_labels() const {
        return *labels;
    }

    // The following methods are used for iterating over the FTS
    FTSConstIterator begin() const {
        return FTSConstIterator(*this, false);
    }

    FTSConstIterator end() const {
        return FTSConstIterator(*this, true);
    }

    int get_size() const {
        return transition_systems.size();
    }

    bool is_active(int index) const {
        return is_index_valid(index);
    }
};
}

#endif
