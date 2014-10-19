#ifndef MERGE_AND_SHRINK_LABELS_H
#define MERGE_AND_SHRINK_LABELS_H

#include <vector>

class EquivalenceRelation;
class Label;
class Options;
class TransitionSystem;

/*
  This class serves both as a container class to handle the set of all labels
  and to perform label reduction on this set.
*/
class Labels {
    std::vector<Label *> labels;

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
        NONE,
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

    LabelReductionMethod label_reduction_method;
    LabelReductionSystemOrder label_reduction_system_order;
    std::vector<int> transition_system_order;

    // Apply the label mapping to all transition systems. Also clear cache.
    void notify_transition_systems(
        int ts_index,
        const std::vector<TransitionSystem *> &all_transition_systems,
        const std::vector<std::pair<int, std::vector<int> > > &label_mapping,
        std::vector<EquivalenceRelation *> &cached_equivalence_relations) const;
    // Apply the given label equivalence relation to the set of labels
    bool apply_label_reduction(
        const EquivalenceRelation *relation,
        std::vector<std::pair<int, std::vector<int> > > &label_mapping);
    EquivalenceRelation *compute_combinable_equivalence_relation(
        int ts_index,
        const std::vector<TransitionSystem *> &all_transition_systems,
        std::vector<EquivalenceRelation *> &cached_local_equivalence_relations) const;

public:
    explicit Labels(const Options &options);
    ~Labels() {}
    void add_label(int label_no, int cost);
    void reduce(std::pair<int, int> next_merge,
                const std::vector<TransitionSystem *> &all_transition_systems);
    bool is_label_reduced(int label_no) const;
    int get_label_cost(int label_no) const;
    void dump_labels() const;
    void dump_label_reduction_options() const;

    int get_size() const {
        return labels.size();
    }
};

#endif
