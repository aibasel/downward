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
    int unsolvable_index; // -1 if solvable, index of an unsolvable entry otw.
    int num_active_entries;

    void compute_distances_and_prune(
        int index,
        Verbosity verbosity);
    void discard_states(
        int index,
        const std::vector<bool> &to_be_pruned_states,
        Verbosity verbosity);
    /*
      Apply the given state equivalence relation to the factor at index if
      it would reduce the size of the factor. Return true iff it was applied
      to the fator.
    */
    bool apply_abstraction(
        int index,
        const StateEquivalenceRelation &state_equivalence_relation,
        Verbosity verbosity);

    void assert_index_valid(int index) const;
    bool is_component_valid(int index) const;
public:
    FactoredTransitionSystem(
        std::unique_ptr<Labels> labels,
        std::vector<std::unique_ptr<TransitionSystem>> &&transition_systems,
        std::vector<std::unique_ptr<MergeAndShrinkRepresentation>> &&mas_representations,
        std::vector<std::unique_ptr<Distances>> &&distances,
        Verbosity verbosity,
        bool finalize_if_unsolvable);
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
      Merge the two factors at index1 and index2. If finalize_if_unsolvable is
      true and the product (the new factor) is unsolvable, the index is stored
      accordingly and get_final_entry() can be called to obtain the final
      distances and MSR.
    */
    int merge(
        int index1,
        int index2,
        Verbosity verbosity,
        bool finalize_if_unsolvable);

    /*
      This method may only be called either when there is only one entry left
      in the FTS or when the FTS is unsolvable.

      Note that the FTS becomes invalid after calling this method.
    */
    std::pair<std::unique_ptr<MergeAndShrinkRepresentation>,
              std::unique_ptr<Distances>> get_final_entry();

    void statistics(int index) const;
    void dump(int index) const;

    const TransitionSystem &get_ts(int index) const {
        return *transition_systems[index];
    }

    const Distances &get_dist(int index) const {
        return *distances[index];
    }

    bool is_solvable() const {
        return unsolvable_index == -1;
    }

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
