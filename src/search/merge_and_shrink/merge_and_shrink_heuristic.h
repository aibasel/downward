#ifndef MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H
#define MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H

#include "../heuristic.h"

class TransitionSystem;
class Labels;
class MergeStrategy;
class ShrinkStrategy;

class MergeAndShrinkHeuristic : public Heuristic {
    MergeStrategy *const merge_strategy;
    ShrinkStrategy *const shrink_strategy;
    bool reduce_labels_before_shrinking;
    bool reduce_labels_before_merging;
    const bool use_expensive_statistics;
    bool terminate;
    Labels *labels;

    TransitionSystem *final_transition_system;
    TransitionSystem *build_transition_system();

    void dump_options() const;
    void warn_on_unusual_options() const;
protected:
    virtual void initialize();
    virtual int compute_heuristic(const GlobalState &state);
public:
    MergeAndShrinkHeuristic(const Options &opts);
    ~MergeAndShrinkHeuristic();
};

#endif
