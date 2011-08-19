#ifndef MAS_HEURISTIC_H
#define MAS_HEURISTIC_H

#include "heuristic.h"
#include "shrink_strategy.h"

#include <utility>
#include <vector>

class Abstraction;

enum MergeStrategy {
    MERGE_LINEAR_CG_GOAL_LEVEL,
    MERGE_LINEAR_CG_GOAL_RANDOM,
    MERGE_LINEAR_GOAL_CG_LEVEL,
    MERGE_LINEAR_RANDOM,
    MERGE_DFP,
    MERGE_LINEAR_LEVEL,
    MERGE_LINEAR_REVERSE_LEVEL,
    MAX_MERGE_STRATEGY
};

class MergeAndShrinkHeuristic : public Heuristic {
    const int max_abstract_states;
    const int max_abstract_states_before_merge;
    const int abstraction_count;
    const MergeStrategy merge_strategy;
    ShrinkStrategy *const shrink_strategy;
    const bool use_label_reduction;
    const bool use_expensive_statistics;

    std::vector<Abstraction *> abstractions;
    std::pair<int, int> compute_shrink_sizes(int size1, int size2) const;
    Abstraction *build_abstraction(bool is_first = true);

    void dump_options() const;
    void warn_on_unusual_options() const;
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    MergeAndShrinkHeuristic(const Options &opts);
    ~MergeAndShrinkHeuristic();
};

#endif
