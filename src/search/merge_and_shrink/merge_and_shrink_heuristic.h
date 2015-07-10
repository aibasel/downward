#ifndef MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H
#define MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H

#include "../heuristic.h"

class Labels;
class MergeStrategy;
class ShrinkStrategy;
class Timer;
class TransitionSystem;

class MergeAndShrinkHeuristic : public Heuristic {
    MergeStrategy *const merge_strategy;
    ShrinkStrategy *const shrink_strategy;
    Labels *labels;
    const bool use_expensive_statistics;
    int starting_peak_memory;

    TransitionSystem *final_transition_system;
    TransitionSystem *build_transition_system(const Timer &timer);

    void report_peak_memory_delta(bool final = false) const;
    void dump_options() const;
    void warn_on_unusual_options() const;
protected:
    virtual void initialize();
    virtual int compute_heuristic(const GlobalState &global_state);
public:
    MergeAndShrinkHeuristic(const Options &opts);
    ~MergeAndShrinkHeuristic();
};

#endif
