#ifndef MERGE_AND_SHRINK_SHRINK_BISIMULATION_H
#define MERGE_AND_SHRINK_SHRINK_BISIMULATION_H

#include "shrink_strategy.h"

class Options;
struct Signature;

class ShrinkBisimulation : public ShrinkStrategy {
    enum AtLimit {
        RETURN,
        USE_UP
    };

    /*
      threshold: Shrink the transition system iff it is larger than this
      size. Note that this is set independently from max_states, which
      is the number of states to which the transition system is shrunk.
    */

    const bool greedy;
    const int threshold;
    const bool group_by_h;
    const AtLimit at_limit;

    void compute_abstraction(TransitionSystem &ts,
                             int target_size,
                             EquivalenceRelation &equivalence_relation);

    int initialize_groups(const TransitionSystem &ts,
                          std::vector<int> &state_to_group);
    void compute_signatures(const TransitionSystem &ts,
                            std::vector<Signature> &signatures,
                            std::vector<int> &state_to_group);
public:
    ShrinkBisimulation(const Options &opts);
    virtual ~ShrinkBisimulation();

    virtual std::string name() const;
    virtual void dump_strategy_specific_options() const;

    virtual bool reduce_labels_before_shrinking() const;

    virtual void shrink(TransitionSystem &ts, int target);
    virtual void shrink_before_merge(TransitionSystem &ts1, TransitionSystem &ts2);
};

#endif
