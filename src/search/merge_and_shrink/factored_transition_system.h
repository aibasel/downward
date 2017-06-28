#ifndef MERGE_AND_SHRINK_FACTORED_TRANSITION_SYSTEM_H
#define MERGE_AND_SHRINK_FACTORED_TRANSITION_SYSTEM_H

#include "types.h"

#include <memory>
#include <vector>

namespace merge_and_shrink {
class Distances;
class FactoredTransitionSystem;
class MergeAndShrinkRepresentation;
class LabelReduction;
class Labels;
class ShrinkStrategy;
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
    std::vector<std::unique_ptr<MergeAndShrinkRepresentation>> mas_representations;
    std::vector<std::unique_ptr<Distances>> distances;
    const bool compute_init_distances;
    const bool compute_goal_distances;
    int num_active_entries;

    /*
      Apply the given state equivalence relation to the factor at index if
      it would reduce the size of the factor. Return true iff it was applied
      to the fator.
    */
    bool apply_abstraction(
        int index,
        const StateEquivalenceRelation &state_equivalence_relation,
        Verbosity verbosity);

    /*
      Assert that the factor at the given index is in a consistent state, i.e.
      that there is a transition system, a distances object, and an MSR.
    */
    void assert_index_valid(int index) const;

    /*
      We maintain the invariant that for all factors, distances are always
      computed and all transitions are grouped according to locally equivalent
      labels.
    */
    bool is_component_valid(int index) const;
public:
    FactoredTransitionSystem(
        std::unique_ptr<Labels> labels,
        std::vector<std::unique_ptr<TransitionSystem>> &&transition_systems,
        std::vector<std::unique_ptr<MergeAndShrinkRepresentation>> &&mas_representations,
        std::vector<std::unique_ptr<Distances>> &&distances,
        const bool compute_init_distances,
        const bool compute_goal_distances,
        Verbosity verbosity);
    FactoredTransitionSystem(FactoredTransitionSystem &&other);
    ~FactoredTransitionSystem();

    // No copying or assignment.
    FactoredTransitionSystem(const FactoredTransitionSystem &) = delete;
    FactoredTransitionSystem &operator=(
        const FactoredTransitionSystem &) = delete;

    // Merge-and-shrink transformations.
    /*
      TODO: this method should be private because it is used internally
      by apply_label_reduction. However, given the iterative algorithm to
      compute label reductions in LabelReduction, it is not easily possible
      to avoid having a method to update the FTS in each iteration.
    */
    void apply_label_mapping(
        const std::vector<std::pair<int, std::vector<int>>> &label_mapping,
        int combinable_index);
    /* Apply a label reduction as computed with the given LabelReduction
       object. */
    bool apply_label_reduction(
        const LabelReduction &label_reduction,
        const std::pair<int, int> &merge_indices,
        Verbosity verbosity);

    /*
      Shrink the transition system of the factor at index to have a size of
      at most target_size, using the given shrink strategy. If the transition
      system was shrunk, update the other components of the factor (distances,
      MSR) and return true.
    */
    bool shrink(
        int index,
        int target_size,
        const ShrinkStrategy &shrink_strategy,
        Verbosity verbosity);

    /*
      Merge the two factors at index1 and index2.
    */
    int merge(
        int index1,
        int index2,
        Verbosity verbosity);

    /*
      Prune unreachable and/or irrelevant states of the factor at index.
      Requires init/goal distances to be computed accordingly. Return true
      iff at least one state was pruned.
    */
    bool prune(
        int index,
        bool prune_unreachable_states,
        bool prune_irrelevant_states,
        Verbosity verbosity);

    /*
      Extract the factor at the given index, rendering the FTS invalid.
    */
    std::pair<std::unique_ptr<MergeAndShrinkRepresentation>,
              std::unique_ptr<Distances>> extract_factor(int index);

    void statistics(int index) const;
    void dump(int index) const;

    const TransitionSystem &get_ts(int index) const {
        return *transition_systems[index];
    }

    const Distances &get_dist(int index) const {
        return *distances[index];
    }

    bool is_factor_solvable(int index) const;

    int get_num_active_entries() const {
        return num_active_entries;
    }

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

    bool is_active(int index) const;
};
}

#endif
