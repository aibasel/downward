#ifndef MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H
#define MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H

#include "../heuristic.h"

#include <memory>

namespace utils {
class Timer;
}

namespace merge_and_shrink {
class FactoredTransitionSystem;
class LabelReduction;
class MergeStrategy;
class ShrinkStrategy;
class TransitionSystem;

class MergeAndShrinkHeuristic : public Heuristic {
    // TODO: when the option parser supports it, the following should become
    // unique pointers.
    std::shared_ptr<MergeStrategy> merge_strategy;
    std::shared_ptr<ShrinkStrategy> shrink_strategy;
    std::shared_ptr<LabelReduction> label_reduction;
    long starting_peak_memory;

    std::unique_ptr<FactoredTransitionSystem> fts;
    void build_transition_system(const utils::Timer &timer);

    void report_peak_memory_delta(bool final = false) const;
    void dump_options() const;
    void warn_on_unusual_options() const;
protected:
    virtual void initialize() override;
    virtual int compute_heuristic(const GlobalState &global_state) override;
public:
    explicit MergeAndShrinkHeuristic(const options::Options &opts);
    ~MergeAndShrinkHeuristic() = default;
};
}

#endif
