#ifndef MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H
#define MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H

#include "../heuristic.h"

#include <memory>

class Labels;
class MergeStrategy;
class ShrinkStrategy;
class Timer;
class TransitionSystem;

class MergeAndShrinkHeuristic : public Heuristic {
    std::shared_ptr<MergeStrategy> merge_strategy;
    std::shared_ptr<ShrinkStrategy> shrink_strategy;
    std::shared_ptr<Labels> labels;
    long starting_peak_memory;

    /*
      TODO: after splitting transition system into several parts, we may
      want to change all transition system pointers into unique_ptr.
    */
    TransitionSystem *final_transition_system;
    void build_transition_system(const Timer &timer);

    void report_peak_memory_delta(bool final = false) const;
    void dump_options() const;
    void warn_on_unusual_options() const;
protected:
    virtual void initialize() override;
    virtual int compute_heuristic(const GlobalState &global_state) override;
public:
    explicit MergeAndShrinkHeuristic(const Options &opts);
    ~MergeAndShrinkHeuristic() = default;
};

#endif
