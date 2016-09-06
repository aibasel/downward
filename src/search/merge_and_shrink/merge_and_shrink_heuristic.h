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
class MergeStrategyFactory;
class ShrinkStrategy;
class TransitionSystem;
enum class Verbosity;

class MergeAndShrinkHeuristic : public Heuristic {
    // TODO: when the option parser supports it, the following should become
    // unique pointers.
    std::shared_ptr<MergeStrategyFactory> merge_strategy_factory;
    std::shared_ptr<ShrinkStrategy> shrink_strategy;
    std::shared_ptr<LabelReduction> label_reduction;

    // Options for shrinking
    // Hard limit: the maximum size of a transition system at any point.
    const int max_states;
    // Hard limit: the maximum size of a transition system before being merged.
    const int max_states_before_merge;
    /*
      A soft limit for triggering shrinking even if the hard limits
      max_states and max_states_before_merge are not violated.
    */
    const int shrink_threshold_before_merge;

    const Verbosity verbosity;

    long starting_peak_memory;
    std::unique_ptr<FactoredTransitionSystem> fts;

    void build_transition_system(const utils::Timer &timer);

    /*
      If any of size1 and size2 is larger than max_states_before_merge or
      if the product of size1 and size2 is larger than max_states, this method
      computes balanced sizes so that these limits are respected again.
    */
    std::pair<int, int> compute_shrink_sizes(int size1, int size2) const;

    /*
      This method checks if the transition system specified via index violates
      the hard size limit (given via new_size) or the soft size limit
      (shrink_threshold_before_merge) and uses the shrink strategy to reduce
      the size of the transition system if necessary.
    */
    bool shrink_transition_system(int index, int new_size);
    std::pair<bool, bool> shrink_before_merge(int index1, int index2);


    void report_peak_memory_delta(bool final = false) const;
    void dump_options() const;
    void warn_on_unusual_options() const;
protected:
    virtual int compute_heuristic(const GlobalState &global_state) override;
public:
    explicit MergeAndShrinkHeuristic(const options::Options &opts);
    ~MergeAndShrinkHeuristic() = default;
    static void add_shrink_limit_options_to_parser(options::OptionParser &parser);
    static void handle_shrink_limit_options_defaults(options::Options &opts);
};
}

#endif
