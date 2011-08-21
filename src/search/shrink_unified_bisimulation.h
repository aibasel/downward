#ifndef SHRINK_UNIFIED_BISIMULATION_H
#define SHRINK_UNIFIED_BISIMULATION_H

// TODO: Once we know we don't need the old classes any more,
//       rename this to shrink_bisimulation.h, also renaming the class
//       and command-line option.

#include "shrink_strategy.h"

class Options;
class Signature;

class ShrinkUnifiedBisimulation : public ShrinkStrategy {
    /*
      greedy: Use greedy bisimulation rather than exact bisimulation.

      threshold: Shrink the abstraction iff it is larger than this
      size. Note that this is set independently from max_states, which
      is the number of states to which the abstraction is shrunk.
    */

    const bool greedy;
    const int threshold;
    const bool skip_atomic_bisimulation;
    const bool initialize_by_h;
    const bool group_by_h;

    std::vector<int> state_to_group;
    std::vector<bool> group_done;
    std::vector<Signature> signatures;

    std::vector<int> h_to_h_group;
    std::vector<bool> h_group_done;

    void compute_abstraction(
        Abstraction &abs,
        int target_size,
        EquivalenceRelation &equivalence_relation);

    int initialize_dfp(const Abstraction &abs);
    int initialize_bisim(const Abstraction &abs);
public:
    ShrinkUnifiedBisimulation(const Options &opts);
    virtual ~ShrinkUnifiedBisimulation();

    virtual std::string name() const;
    virtual void dump_strategy_specific_options() const;

    virtual WhenToNormalize when_to_normalize(bool use_label_reduction) const;

    virtual void shrink(Abstraction &abs, int target, bool force = false);
    virtual void shrink_atomic(Abstraction &abs);
    virtual void shrink_before_merge(Abstraction &abs1, Abstraction &abs2);

    static ShrinkStrategy *create_default();
};

#endif
