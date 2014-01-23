#ifndef MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H
#define MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H

#include "merge_strategy.h"
#include "shrink_strategy.h"

#include "../heuristic.h"

#include <vector>

class Abstraction;
class Labels;

enum LabelReduction {
    NONE,
    APPROXIMATIVE,
    EXACT,
    EXACT_WITH_FIXPOINT
};

class MergeAndShrinkHeuristic : public Heuristic {
    const MergeStrategyEnum merge_strategy_enum;
    ShrinkStrategy *const shrink_strategy;
    const LabelReduction label_reduction;
    const bool use_expensive_statistics;
    Labels *labels;

    Abstraction *final_abstraction;
    Abstraction *build_abstraction();

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
