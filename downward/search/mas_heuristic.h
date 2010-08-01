#ifndef MAS_HEURISTIC_H
#define MAS_HEURISTIC_H

#include "heuristic.h"

#include <vector>
#include <math.h>

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
    const MergeStrategy merge_strategy;
    const ShrinkStrategy shrink_strategy;

    std::vector<Abstraction *> abstractions;
    void verify_no_axioms_no_cond_effects() const;
    Abstraction *build_abstraction(bool is_first = true);
protected:
    virtual void initialize();
    virtual int compute_heuristic(const State &state);
public:
    MergeAndShrinkHeuristic(
        MergeStrategy merge_strategy, ShrinkStrategy shrink_strategy);
    ~MergeAndShrinkHeuristic();
    void dump_options() const;
    static ScalarEvaluator *create(const std::vector<std::string> &config,
                                   int start, int &end);
};

#endif
