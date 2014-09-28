#ifndef MERGE_AND_SHRINK_LABEL_REDUCER_H
#define MERGE_AND_SHRINK_LABEL_REDUCER_H

#include <vector>

class TransitionSystem;
class EquivalenceRelation;
class Label;
class LabelSignature;
class Options;

class LabelReducer {
    /* none: no label reduction will be performed

       two_transition_systems: compute the 'combinable relation'
       for labels only for the two transition_systems that will
       be merged next and reduce labels.

       all_transition_systems: compute the 'combinable relation'
       for labels once for every transition_system and reduce
       labels.

       all_transition_systems_with_fixpoint: keep computing the
       'combinable relation' for labels iteratively for all
       transition systems until no more labels can be reduced.
     */
    enum LabelReductionMethod {
        NONE,
        TWO_TRANSITION_SYSTEMS,
        ALL_TRANSITION_SYSTEMS,
        ALL_TRANSITION_SYSTEMS_WITH_FIXPOINT
    };

    enum LabelReductionSystemOrder {
        REGULAR,
        REVERSE,
        RANDOM
    };

    LabelReductionMethod label_reduction_method;
    LabelReductionSystemOrder label_reduction_system_order;
    std::vector<int> transition_system_order;

    // exact label reduction
    EquivalenceRelation *compute_outside_equivalence(
        int ts_index,
        const std::vector<TransitionSystem *> &all_transition_systems,
        const std::vector<Label *> &labels,
        std::vector<EquivalenceRelation *> &local_equivalence_relations) const;
    // returns true iff at least one new label has been created
    bool reduce_exactly(const EquivalenceRelation *relation,
                        std::vector<Label *> &labels) const;
public:
    explicit LabelReducer(const Options &options);
    ~LabelReducer() {}
    void reduce_labels(std::pair<int, int> next_merge,
                       const std::vector<TransitionSystem *> &all_transition_systems,
                       std::vector<Label * > &labels) const;
    void dump_options() const;
};

#endif
