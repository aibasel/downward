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
      Compute an equivalence relation on the states of the given transition
      system (index in fts) which has a size of at most target (currently
      violated; see issue250). This method needs to be specified by concrete
      shrinking strategies.
    */
    virtual void compute_equivalence_relation(
        const FactoredTransitionSystem &fts,
        int index,
        int target,
        StateEquivalenceRelation &equivalence_relation) const = 0;
    virtual std::string name() const = 0;
    virtual void dump_strategy_specific_options() const = 0;
public:
    ShrinkStrategy();
    virtual ~ShrinkStrategy();

    /*
      Shrink the given transition system (index in fts) so that its size is
      at most target (currently violated; see issue250).

      Note that if target equals the current size of the fts, the shrink
      strategy is not required to actually shrink the size of the transition
      system. However, it may attempt to e.g. shrink the transition system in
      an information preserving way.
    */
    bool shrink(FactoredTransitionSystem &fts, int index, int target);

    void dump_options() const;
    std::string get_name() const;
};
}

#endif
