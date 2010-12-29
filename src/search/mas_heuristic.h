#ifndef MAS_HEURISTIC_H
#define MAS_HEURISTIC_H

#include "heuristic.h"

#include <utility>
#include <vector>

class Abstraction;

enum MergeStrategy {
    MERGE_LINEAR_CG_GOAL_LEVEL,
    MERGE_LINEAR_CG_GOAL_RANDOM,
    MERGE_LINEAR_GOAL_CG_LEVEL,
    MERGE_LINEAR_RANDOM,
    MERGE_DFP,
    MAX_MERGE_STRATEGY
};

enum ShrinkStrategy {
    SHRINK_HIGH_F_LOW_H,
    SHRINK_LOW_F_LOW_H,
    SHRINK_HIGH_F_HIGH_H,
    SHRINK_RANDOM,
    SHRINK_DFP,
    MAX_SHRINK_STRATEGY
};

class MergeAndShrinkHeuristic : public Heuristic {
    const int max_abstract_states;
    const int max_abstract_states_before_merge;
    const int abstraction_count;
    const MergeStrategy merge_strategy;
    const ShrinkStrategy shrink_strategy;
    const bool use_label_simplification;
    const bool use_expensive_statistics;

    std::vector<Abstraction *> abstractions;
    void verify_no_axioms_no_cond_effects() const;
    std::pair<int, int> compute_shrink_sizes(int size1, int size2) const;
    Abstraction *build_abstraction(bool is_first = true);

    void dump_options() const;
    void warn_on_unusual_options() const;
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    MergeAndShrinkHeuristic(
        HeuristicOptions &options,
        int max_abstract_states, int max_abstract_states_before_merge,
        int abstraction_count,
        MergeStrategy merge_strategy, ShrinkStrategy shrink_strategy,
        bool use_label_simplification, bool use_expensive_statistics);
    ~MergeAndShrinkHeuristic();
    static ScalarEvaluator *create(const std::vector<std::string> &config,
                                   int start, int &end, bool dry_run);
};

#endif
