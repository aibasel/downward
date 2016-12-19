#ifndef MERGE_AND_SHRINK_SHRINK_STRATEGY_H
#define MERGE_AND_SHRINK_SHRINK_STRATEGY_H

#include "types.h"

#include <string>
#include <vector>

namespace merge_and_shrink {
class FactoredTransitionSystem;
class TransitionSystem;

class ShrinkStrategy {
protected:
    /*
      Shrink the given transition system (index in fts) with the given
      equivalence relation. This method should be called by all inheriting
      shrink methods at the end of the method "shrink".
    */
    bool shrink_fts(
        FactoredTransitionSystem &fts,
        int index,
        const StateEquivalenceRelation &equivalence_relation,
        Verbosity verbosity) const;
    virtual std::string name() const = 0;
    virtual void dump_strategy_specific_options() const = 0;
public:
    ShrinkStrategy() = default;
    virtual ~ShrinkStrategy() = default;

    /*
      Shrink the given transition system (index in fts) so that its size is
      at most target (currently violated; see issue250).

      Note that if target equals the current size of the fts, the shrink
      strategy is not required to actually shrink the size of the transition
      system. However, it may attempt to e.g. shrink the transition system in
      an information preserving way.
    */
    virtual bool shrink(
        FactoredTransitionSystem &fts,
        int index,
        int target,
        Verbosity verbosity) const = 0;

    void dump_options() const;
    std::string get_name() const;
};
}

#endif
