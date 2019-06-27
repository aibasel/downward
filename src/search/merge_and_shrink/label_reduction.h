#ifndef MERGE_AND_SHRINK_LABEL_REDUCTION_H
#define MERGE_AND_SHRINK_LABEL_REDUCTION_H

#include <memory>
#include <vector>

class TaskProxy;

namespace equivalence_relation {
class EquivalenceRelation;
}

namespace options {
class Options;
}

namespace utils {
class RandomNumberGenerator;
enum class Verbosity;
}

namespace merge_and_shrink {
class FactoredTransitionSystem;

class LabelReduction {
    // Options for label reduction
    std::vector<int> transition_system_order;
    bool lr_before_shrinking;
    bool lr_before_merging;
    /*
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
    std::shared_ptr<utils::RandomNumberGenerator> rng;

    bool initialized() const;
    /* Apply the given label equivalence relation to the set of labels and
       compute the resulting label mapping. */
    void compute_label_mapping(
        const equivalence_relation::EquivalenceRelation *relation,
        const FactoredTransitionSystem &fts,
        std::vector<std::pair<int, std::vector<int>>> &label_mapping,
        utils::Verbosity verbosity) const;
    equivalence_relation::EquivalenceRelation
    *compute_combinable_equivalence_relation(
        int ts_index,
        const FactoredTransitionSystem &fts) const;
public:
    explicit LabelReduction(const options::Options &options);
    void initialize(const TaskProxy &task_proxy);
    bool reduce(
        const std::pair<int, int> &next_merge,
        FactoredTransitionSystem &fts,
        utils::Verbosity verbosity) const;
    void dump_options() const;
    bool reduce_before_shrinking() const {
        return lr_before_shrinking;
    }
    bool reduce_before_merging() const {
        return lr_before_merging;
    }
};
}

#endif
