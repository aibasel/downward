#ifndef MERGE_AND_SHRINK_LABELS_H
#define MERGE_AND_SHRINK_LABELS_H

#include <utility>
#include <vector>

class EquivalenceRelation;
class Label;
class Options;
class TaskProxy;
class TransitionSystem;

/*
  This class serves both as a container class to handle the set of all labels
  and to perform label reduction on this set.
*/
class Labels {
    std::vector<Label *> labels;
    std::vector<int> transition_system_order;

    // Options for label reduction
    bool lr_before_shrinking;
    bool lr_before_merging;
    /*
      none: no label reduction will be performed

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
        TWO_TRANSITION_SYSTEMS,
        ALL_TRANSITION_SYSTEMS,
        ALL_TRANSITION_SYSTEMS_WITH_FIXPOINT
    };
    /*
      Order in which iterations of label reduction considers the set of all
      transition systems. Regular is the fast downward order plus appending
      new composite transition systems after the atomic ones, revers is the
      reversed regulard order and random is a random one. All orders are
      precomputed and reused for every call to reduce().
    */
    enum LabelReductionSystemOrder {
        REGULAR,
        REVERSE,
        RANDOM
    };
    LabelReductionMethod lr_method;
    LabelReductionSystemOrder lr_system_order;

    // Apply the label mapping to all transition systems.
    void notify_transition_systems(int ts_index,
                                   const std::vector<TransitionSystem *> &all_transition_systems,
                                   const std::vector<std::pair<int, std::vector<int> > > &label_mapping) const;
    // Apply the given label equivalence relation to the set of labels
    bool apply_label_reduction(
        const EquivalenceRelation *relation,
        std::vector<std::pair<int, std::vector<int> > > &label_mapping);
    EquivalenceRelation *compute_combinable_equivalence_relation(int ts_index,
                                                                 const std::vector<TransitionSystem *> &all_transition_systems) const;

public:
    explicit Labels(const Options &options);
    ~Labels() {}
    void initialize(const TaskProxy &task_proxy);
    bool initialized() const;
    void add_label(int cost);
    void reduce(std::pair<int, int> next_merge,
                const std::vector<TransitionSystem *> &all_transition_systems);
    bool is_current_label(int label_no) const;
    int get_label_cost(int label_no) const;
    void dump_labels() const;
    void dump_label_reduction_options() const;

    bool reduce_before_shrinking() const {
        return lr_before_shrinking;
    }
    bool reduce_before_merging() const {
        return lr_before_merging;
    }

    int get_size() const {
        return labels.size();
    }
};

#endif
