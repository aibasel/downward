#ifndef MERGE_AND_SHRINK_SHRINK_STRATEGY_H
#define MERGE_AND_SHRINK_SHRINK_STRATEGY_H

#include "types.h"

#include <string>
#include <vector>

namespace merge_and_shrink {
class Distances;
class TransitionSystem;

class ShrinkStrategy {
protected:
    virtual std::string name() const = 0;
    virtual void dump_strategy_specific_options() const = 0;
public:
    ShrinkStrategy() = default;
    virtual ~ShrinkStrategy() = default;

    /*
      Shrink the given transition system so that its size is at most
      target_size (currently violated; see issue250). dist must be the
      distances information associated with the transition system.

      Note that if target_size equals the current size of the transition system,
      the shrink strategy is not required to actually shrink the size of the
      transition system. However, it may attempt to e.g. shrink the transition
      system in an information preserving way.
    */
    virtual StateEquivalenceRelation shrink(
        const TransitionSystem &ts,
        const Distances &distances,
        int target_size) const = 0;

    void dump_options() const;
    std::string get_name() const;
};
}

#endif
