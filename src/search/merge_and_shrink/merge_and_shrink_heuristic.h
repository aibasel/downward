#ifndef MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H
#define MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H

#include "../heuristic.h"

class Abstraction;
class Labels;
class MergeStrategy;
class ShrinkStrategy;

class MergeAndShrinkHeuristic : public Heuristic {
    MergeStrategy *const merge_strategy;
    ShrinkStrategy *const shrink_strategy;
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
