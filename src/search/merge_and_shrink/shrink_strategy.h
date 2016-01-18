#ifndef MERGE_AND_SHRINK_SHRINK_STRATEGY_H
#define MERGE_AND_SHRINK_SHRINK_STRATEGY_H

#include "types.h"

#include <forward_list>
#include <string>
#include <utility>
#include <vector>

namespace options {
class OptionParser;
class Options;
}

namespace merge_and_shrink {
class FactoredTransitionSystem;
class TransitionSystem;

class ShrinkStrategy {
private:
    // Hard limit: the maximum size of a transition system at any point.
    const int max_states;
    // Hard limit: the maximum size of a transition system before being merged.
    const int max_states_before_merge;
    /*
      A soft limit for triggering shrinking even if the hard limits
      max_states and max_states_before_merge are not violated.
    */
    const int shrink_threshold_before_merge;

    /*
      This method checks if the transition system violates the hard or soft
      limits and triggers the actual shrinking process if necessary.

      Note that if only the soft limit shrink_threshold_before_merge is
      violated, the  shrink strategy is asked to compute an equivalence
      relation which is not required to reduce the sizes of the transition
      system, but it may attempt to e.g. shrink the transition system in an
      information preserving way.
    */
    bool shrink_transition_system(FactoredTransitionSystem &fts,
                                  int index, int new_size) const;
    /*
      If max_states_before_merge is violated by any of the two transition
      systems or if the product transition system would exceed max_states,
      this method computes balanced sizes for both transition systems so
      that these limits are respected again.
    */
    std::pair<std::size_t, std::size_t> compute_shrink_sizes(
        std::size_t size1, std::size_t size2) const;
protected:
    /*
      Compute an equivalence relation on the states that shrinks the given
      transition system down to at most size target. This method needs to be
      specified by concrete shrinking strategies.
    */
    virtual void compute_equivalence_relation(
        const FactoredTransitionSystem &fts,
        int index,
        int target,
        StateEquivalenceRelation &equivalence_relation) const = 0;
    virtual std::string name() const = 0;
    virtual void dump_strategy_specific_options() const = 0;
public:
    ShrinkStrategy(const options::Options &opts);
    virtual ~ShrinkStrategy();

    /*
      The given transition systems are guaranteed to be solvable by the
      merge-and-shrink computation.
    */
    std::pair<bool, bool> shrink(FactoredTransitionSystem &fts,
                                 int index1,
                                 int index2) const;

    void dump_options() const;
    std::string get_name() const;

    static void add_options_to_parser(options::OptionParser &parser);
    static void handle_option_defaults(options::Options &opts);
};
}

#endif
