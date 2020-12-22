#ifndef MERGE_AND_SHRINK_SHRINK_STRATEGY_H
#define MERGE_AND_SHRINK_SHRINK_STRATEGY_H

#include "types.h"

#include <string>
#include <vector>

namespace utils {
class LogProxy;
}

namespace merge_and_shrink {
class Distances;
class TransitionSystem;

class ShrinkStrategy {
protected:
    virtual std::string name() const = 0;
    virtual void dump_strategy_specific_options(utils::LogProxy &log) const = 0;
public:
    ShrinkStrategy() = default;
    virtual ~ShrinkStrategy() = default;

    /*
      Compute a state equivalence relation over the states of the given
      transition system such that its new number of states after abstracting
      it according to this equivalence relation is at most target_size
      (currently violated; see issue250). dist must be the distances
      information associated with the given transition system.

      Note that if target_size equals the current size of the transition system,
      the shrink strategy is not required to compute an equivalence relation
      that results in actually shrinking the size of the transition system.
      However, it may attempt to e.g. compute an equivalence relation that
      results in shrinking the transition system in an information-preserving
      way.
    */
    virtual StateEquivalenceRelation compute_equivalence_relation(
        const TransitionSystem &ts,
        const Distances &distances,
        int target_size,
        utils::LogProxy &log) const = 0;
    virtual bool requires_init_distances() const = 0;
    virtual bool requires_goal_distances() const = 0;

    void dump_options(utils::LogProxy &log) const;
    std::string get_name() const;
};
}

#endif
