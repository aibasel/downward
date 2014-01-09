#ifndef MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H
#define MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H

#include "equivalence_relation.h"
#include "shrink_strategy.h"

#include "../heuristic.h"

#include <vector>

class Abstraction;
class Labels;

enum MergeStrategy {
    MERGE_LINEAR_CG_GOAL_LEVEL,
    MERGE_LINEAR_CG_GOAL_RANDOM,
    MERGE_LINEAR_GOAL_CG_LEVEL,
    MERGE_LINEAR_RANDOM,
    MERGE_DFP,
    MERGE_LINEAR_LEVEL,
    MERGE_LINEAR_REVERSE_LEVEL
};

class MergeAndShrinkHeuristic : public Heuristic {
    const MergeStrategy merge_strategy;
    ShrinkStrategy *const shrink_strategy;
    const bool exact_label_reduction;
    const bool use_expensive_statistics;
    Labels *labels;

    Abstraction *final_abstraction;
    Abstraction *build_abstraction();

    void dump_options() const;
    void warn_on_unusual_options() const;
    EquivalenceRelation *compute_outside_equivalence(const Abstraction *abstraction,
                                                     const std::vector<Abstraction *> &all_abstractions) const;
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    MergeAndShrinkHeuristic(const Options &opts);
    ~MergeAndShrinkHeuristic();
};

#endif
