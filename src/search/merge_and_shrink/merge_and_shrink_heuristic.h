#ifndef MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H
#define MERGE_AND_SHRINK_MERGE_AND_SHRINK_HEURISTIC_H

#include "../heuristic.h"

#include <memory>

namespace utils {
class Timer;
}

namespace merge_and_shrink {
class FactorScoringFunction;
class FactoredTransitionSystem;
class LabelReduction;
class MergeAndShrinkRepresentation;
class MergeStrategyFactory;
class ShrinkStrategy;
class TransitionSystem;
enum class Verbosity;

enum class PartialMASMethod {
    None,
    Single,
    Maximum
};

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
    /* A soft limit for triggering shrinking even if the hard limits
       max_states and max_states_before_merge are not violated. */
    const int shrink_threshold_before_merge;

    // Options for pruning
    const bool prune_unreachable_states;
    const bool prune_irrelevant_states;

    const Verbosity verbosity;

    // Options related to computing partial abstractions
    const double max_time;
    const int num_transitions_to_abort;
    const PartialMASMethod partial_mas_method;
    std::vector<std::shared_ptr<FactorScoringFunction>> factor_scoring_functions;

    long starting_peak_memory;
    // The final merge-and-shrink representations, storing goal distances.
    std::vector<std::unique_ptr<MergeAndShrinkRepresentation>> mas_representations;

    bool ran_out_of_time(const utils::Timer &timer) const;
    bool too_many_transitions(int num_transitions) const;
    int find_best_factor(const FactoredTransitionSystem &fts) const;
    void finalize_factor(FactoredTransitionSystem &fts, int index);
    void finalize(FactoredTransitionSystem &fts);
    int prune_fts(FactoredTransitionSystem &fts, const utils::Timer &timer) const;
    int main_loop(FactoredTransitionSystem &fts, const utils::Timer &timer);
    void build(const utils::Timer &timer);

    void report_peak_memory_delta(bool final = false) const;
    void dump_options() const;
    void warn_on_unusual_options() const;
protected:
    virtual int compute_heuristic(const GlobalState &global_state) override;
public:
    explicit MergeAndShrinkHeuristic(const options::Options &opts);
    virtual ~MergeAndShrinkHeuristic() override = default;
    static void add_shrink_limit_options_to_parser(options::OptionParser &parser);
    static void handle_shrink_limit_options_defaults(options::Options &opts);
};
}

#endif
